/*
 * Copyright (C) 2026 Trond
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* "Open in MuseScore": a MIDI region is written out as a small MIDI file,
 * turned into a MuseScore score and opened in MuseScore. Every time the
 * score is saved there, its notes are read back into the region.
 *
 * Files live in <session>/score/. The MIDI file we wrote is kept beside the
 * score: if the region still matches it next time, the existing score (with
 * its layout, lyrics and so on) is opened again instead of a fresh one.
 */

#include <cmath>
#include <cstdint>
#include <list>
#include <map>
#include <string>
#include <vector>

#include <glib/gstdio.h>
#include <glibmm/fileutils.h>
#include <glibmm/main.h>
#include <glibmm/miscutils.h>
#include <glibmm/spawn.h>

#include <ytkmm/label.h>
#include <ytkmm/stock.h>

#include "pbd/stateful_diff_command.h"

#include "temporal/tempo.h"

#include "ardour/midi_model.h"
#include "ardour/midi_region.h"
#include "ardour/midi_source.h"
#include "ardour/session.h"

#include "ardour_dialog.h"
#include "ardour_message.h"
#include "editor.h"
#include "mouse_cursors.h"
#include "region_view.h"
#include "utils.h"

#include "pbd/i18n.h"

using namespace ARDOUR;
using namespace Temporal;
using std::string;

namespace {

const int ppqn = 480;

struct ScoreNote {
	int64_t on;  /* ticks from the start of the file */
	int64_t len; /* ticks */
	uint8_t chan, pitch, vel;
};

/* one region that is open in MuseScore */
struct ScoreLink {
	std::weak_ptr<MidiRegion> region;
	string  mid;   /* what we sent */
	string  score; /* what MuseScore edits */
	string  back;  /* the score, converted back to MIDI */
	int64_t lead;  /* ticks of empty space before the region, so bars line up */
	gint64  mtime;
	gint64  size;
};

std::list<ScoreLink> links;
sigc::connection     watch;

/* ---- a minimal Standard MIDI File writer and reader ---- */

void
put_var (string& s, uint32_t v)
{
	uint8_t b[5];
	int     n = 0;
	b[n++]    = v & 0x7f;
	while (v >>= 7) {
		b[n++] = 0x80 | (v & 0x7f);
	}
	while (n) {
		s += (char) b[--n];
	}
}

void
put_be (string& s, uint32_t v, int bytes)
{
	for (int i = bytes - 1; i >= 0; --i) {
		s += (char) ((v >> (8 * i)) & 0xff);
	}
}

string
smf_bytes (std::vector<ScoreNote> const& notes, string const& name, double bpm, int num, int den, int64_t end)
{
	struct Ev {
		int64_t t;
		int     order; /* note-offs before note-ons at the same time */
		uint8_t d[3];
	};
	std::vector<Ev> evs;
	for (ScoreNote const& n : notes) {
		evs.push_back ({ n.on, 1, { (uint8_t) (0x90 | n.chan), n.pitch, n.vel } });
		evs.push_back ({ n.on + std::max<int64_t> (1, n.len), 0, { (uint8_t) (0x80 | n.chan), n.pitch, 0 } });
	}
	std::stable_sort (evs.begin (), evs.end (), [] (Ev const& a, Ev const& b) {
		return a.t != b.t ? a.t < b.t : a.order < b.order;
	});

	string trk;
	/* track name */
	trk += '\0';
	trk += "\xff\x03";
	put_var (trk, name.size ());
	trk += name;
	/* time signature */
	int dd = 0;
	while ((1 << dd) < den) {
		++dd;
	}
	trk += '\0';
	trk += "\xff\x58\x04";
	trk += (char) num;
	trk += (char) dd;
	trk += (char) 24;
	trk += (char) 8;
	/* tempo */
	trk += '\0';
	trk += "\xff\x51\x03";
	put_be (trk, (uint32_t) llround (60000000.0 / bpm), 3);

	int64_t now = 0;
	for (Ev const& e : evs) {
		put_var (trk, (uint32_t) (e.t - now));
		now = e.t;
		trk.append ((const char*) e.d, 3);
	}
	put_var (trk, (uint32_t) std::max<int64_t> (0, end - now));
	trk += "\xff\x2f";
	trk += '\0';

	string f ("MThd");
	put_be (f, 6, 4);
	put_be (f, 0, 2);
	put_be (f, 1, 2);
	put_be (f, ppqn, 2);
	f += "MTrk";
	put_be (f, trk.size (), 4);
	f += trk;
	return f;
}

/* read the notes of all tracks; times are scaled to our ppqn */
bool
smf_read (string const& path, std::vector<ScoreNote>& notes)
{
	string d;
	try {
		d = Glib::file_get_contents (path);
	} catch (...) {
		return false;
	}
	const uint8_t* p   = (const uint8_t*) d.data ();
	const size_t   len = d.size ();
	if (len < 14 || d.compare (0, 4, "MThd")) {
		return false;
	}
	const int div = (p[12] << 8) | p[13];
	if (div <= 0 || (div & 0x8000)) {
		return false;
	}
	size_t pos = 8 + ((p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7]);

	while (pos + 8 <= len) {
		const bool   is_track = !d.compare (pos, 4, "MTrk");
		const size_t clen     = ((size_t) p[pos + 4] << 24) | (p[pos + 5] << 16) | (p[pos + 6] << 8) | p[pos + 7];
		pos += 8;
		const size_t end = std::min (len, pos + clen);
		if (!is_track) {
			pos = end;
			continue;
		}
		int64_t t      = 0;
		uint8_t status = 0;
		std::map<int, std::pair<int64_t, uint8_t> > sounding; /* chan*128+pitch -> start, velocity */

		auto var = [&] () {
			uint32_t v = 0;
			while (pos < end) {
				const uint8_t b = p[pos++];
				v = (v << 7) | (b & 0x7f);
				if (!(b & 0x80)) {
					break;
				}
			}
			return v;
		};

		while (pos < end) {
			t += var ();
			if (pos >= end) {
				break;
			}
			uint8_t b = p[pos];
			if (b == 0xff) {
				pos += 2;
				pos += var ();
				continue;
			}
			if (b == 0xf0 || b == 0xf7) {
				++pos;
				pos += var ();
				continue;
			}
			if (b & 0x80) {
				status = b;
				++pos;
			}
			const uint8_t type = status & 0xf0;
			const uint8_t chan = status & 0x0f;
			if (type == 0xc0 || type == 0xd0) {
				++pos;
				continue;
			}
			if (pos + 1 >= end) {
				break;
			}
			const uint8_t d1 = p[pos], d2 = p[pos + 1];
			pos += 2;
			const int key = chan * 128 + d1;
			if (type == 0x90 && d2 > 0) {
				sounding[key] = std::make_pair (t, d2);
			} else if (type == 0x80 || type == 0x90) {
				auto s = sounding.find (key);
				if (s != sounding.end ()) {
					ScoreNote n;
					n.on    = llround ((double) s->second.first * ppqn / div);
					n.len   = std::max<int64_t> (1, llround ((double) (t - s->second.first) * ppqn / div));
					n.chan  = chan;
					n.pitch = d1;
					n.vel   = s->second.second;
					notes.push_back (n);
					sounding.erase (s);
				}
			}
		}
		pos = end;
	}
	return true;
}

/* ---- MuseScore ---- */

string
find_musescore ()
{
	static const char* names[] = { "mscore", "musescore", "mscore4", "musescore4", "mscore3", "musescore3",
		                       "mscore4portable", "MuseScore", 0 };
	if (const char* e = g_getenv ("ARDOUR_MUSESCORE")) {
		return e;
	}
	for (int i = 0; names[i]; ++i) {
		const string p = Glib::find_program_in_path (names[i]);
		if (!p.empty ()) {
			return p;
		}
	}
	return string ();
}

bool
convert (string const& exe, string const& from, string const& to)
{
	std::vector<string> argv = { exe, "-o", to, from };
	int                 status = 0;
	string              out, err;
	::g_unlink (to.c_str ());
	try {
		Glib::spawn_sync ("", argv, Glib::SpawnFlags (0), sigc::slot<void> (), &out, &err, &status);
	} catch (...) {
		return false;
	}
	return Glib::file_test (to, Glib::FILE_TEST_EXISTS);
}

bool
file_stamp (string const& path, gint64& mtime, gint64& size)
{
	GStatBuf st;
	if (g_stat (path.c_str (), &st)) {
		return false;
	}
	mtime = st.st_mtime;
	size  = st.st_size;
	return true;
}

/* ---- region <-> notes ---- */

/* empty space to put before the region so that bar lines fall in the right place */
int64_t
lead_ticks (std::shared_ptr<MidiRegion> r)
{
	TempoMap::SharedPtr tmap = TempoMap::use ();
	const Beats         pos  = r->position ().beats ();
	BBT_Argument        bbt  = tmap->bbt_at (pos);
	const Beats         bar  = tmap->quarters_at (BBT_Argument (bbt.reference (), bbt.bars, 1, 0));
	return std::max<int64_t> (0, (pos - bar).to_ticks (ppqn));
}

void
region_notes (std::shared_ptr<MidiRegion> r, int64_t lead, std::vector<ScoreNote>& out)
{
	std::shared_ptr<MidiModel> model = r->midi_source (0)->model ();
	if (!model) {
		return;
	}
	const Beats start = r->start ().beats ();
	const Beats end   = start + r->length ().beats ();
	MidiModel::ReadLock lock (model->read_lock ());
	for (auto const& n : model->notes ()) {
		if (n->time () < start || n->time () >= end) {
			continue;
		}
		ScoreNote s;
		s.on    = lead + (n->time () - start).to_ticks (ppqn);
		s.len   = n->length ().to_ticks (ppqn);
		s.chan  = n->channel ();
		s.pitch = n->note ();
		s.vel   = std::max<uint8_t> (1, n->velocity ());
		out.push_back (s);
	}
}

string
region_smf (std::shared_ptr<MidiRegion> r, int64_t lead)
{
	std::vector<ScoreNote> notes;
	region_notes (r, lead, notes);
	TempoMap::SharedPtr tmap = TempoMap::use ();
	Temporal::MeterPoint const& m    = tmap->meter_at (r->position ());
	Temporal::TempoPoint const& t    = tmap->tempo_at (r->position ());
	return smf_bytes (notes, r->name (), t.quarter_notes_per_minute (), m.divisions_per_bar (), m.note_value (),
	                  lead + r->length ().beats ().to_ticks (ppqn));
}

/* replace the region's notes with those of a MIDI file (one undo step) */
bool
notes_from_file (Session& session, ScoreLink const& l, std::shared_ptr<MidiRegion> r)
{
	std::vector<ScoreNote> in;
	if (!smf_read (l.back, in)) {
		return false;
	}
	std::shared_ptr<MidiModel> model = r->midi_source (0)->model ();
	if (!model) {
		return false;
	}
	const Beats start = r->start ().beats ();
	Beats       end   = start + r->length ().beats ();

	/* MuseScore numbers MIDI channels its own way: a region that used one
	 * channel keeps it */
	int only_chan = -1;
	MidiModel::NoteDiffCommand* cmd = model->new_note_diff_command (_("edit in MuseScore"));
	{
		MidiModel::ReadLock lock (model->read_lock ());
		for (auto const& n : model->notes ()) {
			if (n->time () < start || n->time () >= end) {
				continue;
			}
			only_chan = (only_chan == -1 || only_chan == n->channel ()) ? n->channel () : -2;
			cmd->remove (n);
		}
	}

	Beats last = end;
	for (ScoreNote const& s : in) {
		if (s.on < l.lead) {
			continue; /* written before the start of the region */
		}
		const Beats t   = start + Beats::ticks_at_rate (s.on - l.lead, ppqn);
		const Beats len = Beats::ticks_at_rate (s.len, ppqn);
		const uint8_t c = only_chan >= 0 ? only_chan : s.chan;
		cmd->add (MidiModel::NotePtr (new Evoral::Note<Beats> (c, t, len, s.pitch, s.vel)));
		last = std::max (last, t + len);
	}

	session.begin_reversible_command (_("edit in MuseScore"));
	if (last > end) {
		/* bars were added in the score: make the region longer, to the next bar */
		TempoMap::SharedPtr tmap = TempoMap::use ();
		const Beats         abs  = r->position ().beats () + (last - start);
		BBT_Argument        bbt  = tmap->bbt_at (abs);
		Beats               bar  = tmap->quarters_at (BBT_Argument (bbt.reference (), bbt.bars, 1, 0));
		if (bar < abs) {
			bar = tmap->quarters_at (BBT_Argument (bbt.reference (), bbt.bars + 1, 1, 0));
		}
		r->clear_changes ();
		r->set_length (timecnt_t (bar - r->position ().beats (), r->position ()));
		session.add_command (new PBD::StatefulDiffCommand (r));
	}
	model->apply_diff_command_as_subcommand (session, cmd);
	session.commit_reversible_command ();
	return true;
}

} // namespace

void
Editor::open_region_in_musescore ()
{
	if (!_session) {
		return;
	}
	std::shared_ptr<MidiRegion> mr;
	for (RegionView* rv : get_regions_from_selection_and_entered ()) {
		if ((mr = std::dynamic_pointer_cast<MidiRegion> (rv->region ()))) {
			break;
		}
	}
	if (!mr) {
		return;
	}

	const string exe = find_musescore ();
	if (exe.empty ()) {
		ArdourDialog d (_("MuseScore is not installed"), true);
		Gtk::Label*  a = manage (new Gtk::Label (_("Ardour shows and prints notes with MuseScore, a free notation program.\n"
		                                           "To install it, open a terminal and paste this line:")));
		Gtk::Label*  c = manage (new Gtk::Label);
		c->set_markup ("<tt><b>sudo apt install musescore3</b></tt>");
		c->set_selectable (true);
		Gtk::Label* b = manage (new Gtk::Label (_("Then choose Open in MuseScore again.")));
		for (Gtk::Label* l : { a, c, b }) {
			l->set_alignment (0.0, 0.5);
			d.get_vbox ()->pack_start (*l, false, false);
		}
		d.get_vbox ()->set_spacing (10);
		d.get_vbox ()->set_border_width (16);
		d.add_button (Gtk::Stock::OK, Gtk::RESPONSE_OK);
		d.get_vbox ()->show_all ();
		ARDOUR_UI_UTILS::enlarge_fonts (d, 1.2);
		d.run ();
		return;
	}

	const string dir = Glib::build_filename (_session->path (), "score");
	g_mkdir_with_parents (dir.c_str (), 0755);

	string base = mr->name ();
	for (char& ch : base) {
		if (!g_ascii_isalnum (ch) && ch != '-' && ch != '_' && ch != '.') {
			ch = '_';
		}
	}
	base = Glib::build_filename (dir, base + "-" + mr->id ().to_s ());

	ScoreLink l;
	l.region = mr;
	l.mid    = base + ".mid";
	l.score  = base + ".mscz";
	l.back   = base + ".back.mid";
	l.lead   = lead_ticks (mr);

	const string bytes = region_smf (mr, l.lead);
	string       before;
	try {
		before = Glib::file_get_contents (l.mid);
	} catch (...) {
	}

	if (before != bytes || !Glib::file_test (l.score, Glib::FILE_TEST_EXISTS)) {
		/* new, or changed in Ardour since last time: make a fresh score */
		CursorRAII busy (*this, cursors ()->wait);
		gdk_flush ();
		try {
			Glib::file_set_contents (l.mid, bytes);
		} catch (...) {
			return;
		}
		if (!convert (exe, l.mid, l.score)) {
			ArdourMessageDialog (_("MuseScore could not make a score from this region.")).run ();
			return;
		}
	}

	if (!file_stamp (l.score, l.mtime, l.size)) {
		return;
	}

	try {
		std::vector<string> argv = { exe, l.score };
		Glib::spawn_async ("", argv, Glib::SpawnFlags (0));
	} catch (...) {
		ArdourMessageDialog (_("MuseScore could not be started.")).run ();
		return;
	}

	for (auto i = links.begin (); i != links.end ();) {
		i = (i->score == l.score) ? links.erase (i) : ++i;
	}
	links.push_back (l);
	if (!watch.connected ()) {
		watch = Glib::signal_timeout ().connect (sigc::mem_fun (*this, &Editor::musescore_poll), 1000);
	}
}

/* once a second: has a score been saved in MuseScore? */
bool
Editor::musescore_poll ()
{
	for (auto i = links.begin (); i != links.end ();) {
		std::shared_ptr<MidiRegion> r = i->region.lock ();
		if (!r || !_session) {
			i = links.erase (i);
			continue;
		}
		gint64 mtime, size;
		if (file_stamp (i->score, mtime, size) && size > 0 && (mtime != i->mtime || size != i->size)) {
			i->mtime = mtime;
			i->size  = size;
			const string exe = find_musescore ();
			if (!exe.empty () && convert (exe, i->score, i->back) && notes_from_file (*_session, *i, r)) {
				/* remember what the region now holds, so that the same score is reused next time */
				try {
					Glib::file_set_contents (i->mid, region_smf (r, i->lead));
				} catch (...) {
				}
			}
		}
		++i;
	}
	if (links.empty ()) {
		watch.disconnect ();
		return false;
	}
	return true;
}
