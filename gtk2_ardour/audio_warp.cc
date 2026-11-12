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

#include <algorithm>
#include <ctime>
#include <set>
#include <sstream>

#include <rubberband/RubberBandStretcher.h>

#include "pbd/basename.h"
#include "pbd/error.h"
#include "pbd/failed_constructor.h"
#include "pbd/stateful_diff_command.h"
#include "pbd/xml++.h"

#include "temporal/tempo.h"

#include "ardour/analyser.h"
#include "ardour/playlist.h"
#include "ardour/audiofilesource.h"
#include "ardour/audioregion.h"
#include "ardour/region_factory.h"
#include "ardour/session.h"
#include "ardour/source_factory.h"
#include "ardour/onset_detector.h"

#include "audio_warp.h"

#include "pbd/i18n.h"

using namespace ARDOUR;
using namespace PBD;
using std::shared_ptr;
using std::string;

static const char* const warp_node_name = "AudioWarp";

shared_ptr<AudioRegion>
AudioWarp::original (shared_ptr<AudioRegion> r, Map& markers)
{
	markers.clear ();

	XMLNode* node = r->extra_xml (warp_node_name);
	if (!node) {
		return r;
	}

	Session&    s = r->session ();
	SourceList  sl;
	std::string ids;
	samplepos_t start  = 0;
	samplecnt_t length = 0;

	if (!node->get_property (X_("sources"), ids) || !node->get_property (X_("start"), start) || !node->get_property (X_("length"), length)) {
		return r;
	}

	std::stringstream ss (ids);
	std::string       id;
	while (std::getline (ss, id, ',')) {
		shared_ptr<Source> src = s.source_by_id (PBD::ID (id));
		if (!src) {
			return r; /* original audio gone: treat this as an unwarped region */
		}
		sl.push_back (src);
	}

	for (XMLNode const* c : node->children ()) {
		Marker m;
		if (c->get_property (X_("in"), m.in) && c->get_property (X_("out"), m.out)) {
			markers.push_back (m);
		}
	}

	PropertyList plist;
	plist.add (Properties::start, timepos_t (start));
	plist.add (Properties::length, timecnt_t (length));
	plist.add (Properties::name, r->name ());
	plist.add (Properties::whole_file, false);

	shared_ptr<AudioRegion> orig = std::dynamic_pointer_cast<AudioRegion> (RegionFactory::create (sl, plist, false));
	if (!orig) {
		markers.clear ();
		return r;
	}
	return orig;
}

void
AudioWarp::detect_hits (shared_ptr<AudioRegion> r, std::vector<samplepos_t>& hits)
{
	/* note/hit starts, as Rhythm Ferret's "note onset" analysis, but stricter
	 * so the fading tail of a drum hit is not taken for another hit */
	const float sr = r->session ().sample_rate ();
	try {
		OnsetDetector       t (sr);
		AnalysisFeatureList all;
		for (uint32_t c = 0; c < r->n_channels (); ++c) {
			AnalysisFeatureList these;
			t.set_function (3);              /* complex domain */
			t.set_silence_threshold (-60.0); /* dB: ignore the quiet tails */
			t.set_peak_threshold (0.5);
			t.set_minioi (40);               /* ms between hits */
			t.reset ();
			if (t.run ("", r.get (), c, these)) {
				continue;
			}
			all.insert (all.end (), these.begin (), these.end ());
		}
		OnsetDetector::cleanup_onsets (all, sr, 50.0 /* ms */);
		for (samplepos_t p : all) {
			hits.push_back (p); /* relative to the region start */
		}
	} catch (failed_constructor&) {
		error << _("Could not load the hit detection plugin") << endmsg;
	}
}

shared_ptr<AudioRegion>
AudioWarp::render (shared_ptr<AudioRegion> orig, shared_ptr<AudioRegion> placed_like, Map const& markers)
{
	using namespace RubberBand;

	shared_ptr<AudioRegion> rv;
	Session&                session  = orig->session ();
	const samplecnt_t       sr       = session.sample_rate ();
	const uint32_t          nchan    = orig->n_channels ();
	const samplecnt_t       len      = orig->length_samples ();
	const samplecnt_t       bufsize  = 8192;

	RubberBandStretcher rbs (sr, nchan, RubberBandStretcher::PercussiveOptions | RubberBandStretcher::OptionProcessOffline, 1.0, 1.0);
	rbs.setExpectedInputDuration (len);

	std::map<size_t, size_t> keyframes;
	for (Marker const& m : markers) {
		keyframes[m.in] = m.out;
	}

	/* new files, named after the originals */
	std::vector<shared_ptr<AudioSource>> out;
	std::vector<string>                  names = orig->master_source_names ();
	for (uint32_t c = 0; c < nchan; ++c) {
		string base = PBD::basename_nosuffix (c < names.size () ? names[c] : orig->name ()) + "(warp)";
		string path = session.new_audio_source_path (base, nchan, c, false);
		if (path.empty ()) {
			return rv;
		}
		try {
			out.push_back (std::dynamic_pointer_cast<AudioSource> (SourceFactory::createWritable (DataType::AUDIO, session, path, sr)));
		} catch (failed_constructor&) {
			for (auto& s : out) { s->mark_for_remove (); }
			return rv;
		}
	}

	std::vector<std::vector<float>> data (nchan, std::vector<float> (bufsize));
	std::vector<float*>             bufs (nchan);
	for (uint32_t c = 0; c < nchan; ++c) {
		bufs[c] = &data[c][0];
	}

	auto retrieve = [&] () {
		int avail;
		while ((avail = rbs.available ()) > 0) {
			const samplecnt_t n = std::min<samplecnt_t> (bufsize, avail);
			rbs.retrieve (&bufs[0], n);
			for (uint32_t c = 0; c < nchan; ++c) {
				out[c]->write (bufs[c], n);
			}
		}
	};

	for (int pass = 0; pass < 2; ++pass) {
		if (pass == 1) {
			if (!keyframes.empty ()) {
				rbs.setKeyFrameMap (keyframes);
			}
		}
		for (samplecnt_t pos = 0; pos < len;) {
			const samplecnt_t n = std::min (bufsize, len - pos);
			for (uint32_t c = 0; c < nchan; ++c) {
				orig->read (bufs[c], pos, n, c); /* raw audio, no fades or gain */
			}
			pos += n;
			if (pass == 0) {
				rbs.study (&bufs[0], n, pos == len);
			} else {
				rbs.process (&bufs[0], n, pos == len);
				retrieve ();
			}
		}
	}
	retrieve ();

	/* finish the files (as LuaAPI::Rubberband::finalize) */
	time_t     xnow = time (NULL);
	struct tm* now  = localtime (&xnow);
	SourceList sl;
	for (auto& s : out) {
		shared_ptr<AudioFileSource> afs = std::dynamic_pointer_cast<AudioFileSource> (s);
		afs->done_with_peakfile_writes ();
		afs->update_header (placed_like->position_sample (), *now, xnow);
		afs->mark_immutable ();
		Analyser::queue_source_for_analysis (s, false);
		sl.push_back (s);
	}

	/* A warped region that was trimmed or split since stays trimmed: the new
	 * files cover the same stretch of time as the ones they replace. */
	const bool        was_warped = placed_like->extra_xml (warp_node_name) != 0;
	const samplepos_t keep_start = was_warped ? std::max<samplepos_t> (0, std::min<samplepos_t> (placed_like->start_sample (), len - 1)) : 0;
	const samplecnt_t keep_len   = was_warped ? std::max<samplecnt_t> (1, std::min<samplecnt_t> (placed_like->length_samples (), len - keep_start)) : len;

	PropertyList plist (placed_like->derive_properties ());
	plist.add (Properties::start, timepos_t (keep_start));
	plist.add (Properties::length, timecnt_t (keep_len));
	plist.add (Properties::name, placed_like->name ());
	plist.add (Properties::whole_file, false);

	rv = std::dynamic_pointer_cast<AudioRegion> (RegionFactory::create (sl, plist));
	if (!rv) {
		return rv;
	}

	/* remember the original and the markers */
	XMLNode*    node = new XMLNode (warp_node_name);
	std::string ids;
	for (uint32_t c = 0; c < orig->n_channels (); ++c) {
		if (!ids.empty ()) {
			ids += ",";
		}
		ids += orig->source (c)->id ().to_s ();
	}
	node->set_property (X_("sources"), ids);
	node->set_property (X_("start"), orig->start_sample ());
	node->set_property (X_("length"), len);
	for (Marker const& m : markers) {
		XMLNode* c = node->add_child (X_("Marker"));
		c->set_property (X_("in"), m.in);
		c->set_property (X_("out"), m.out);
	}
	rv->add_extra_xml (*node);
	return rv;
}

/* Where the first sample of a region's (warped) audio file sits on the
 * timeline. For a warped region that was trimmed at the front this is before
 * the region's own position. */
static samplepos_t
file_position (shared_ptr<AudioRegion> r)
{
	return r->position_sample () - (r->extra_xml (warp_node_name) ? r->start_sample () : 0);
}

/* swap `old` for `nr` in its playlist, as part of the current undo step */
static void
replace_in_playlist (shared_ptr<AudioRegion> old, shared_ptr<AudioRegion> nr, std::set<shared_ptr<Playlist> >& touched)
{
	shared_ptr<Playlist> pl = old->playlist ();
	if (!pl) {
		return;
	}
	if (touched.insert (pl).second) {
		pl->clear_changes ();
	}
	pl->replace_region (old, nr, old->position ());
}

void
AudioWarp::quantize (std::vector<shared_ptr<AudioRegion> > const& regions, Temporal::Beats grid, double strength)
{
	if (regions.empty () || grid <= Temporal::Beats ()) {
		return;
	}

	Session&          session = regions.front ()->session ();
	const samplecnt_t sr      = session.sample_rate ();
	const samplecnt_t gap     = sr / 50; /* 20 ms: markers closer than this are merged */

	/* 1. the hits of all regions, on the timeline, merged into one guide */
	std::vector<shared_ptr<AudioRegion> > originals;
	std::vector<samplepos_t>               guide;
	for (shared_ptr<AudioRegion> const& r : regions) {
		Map old;
		shared_ptr<AudioRegion> orig = original (r, old);
		originals.push_back (orig);
		std::vector<samplepos_t> hits;
		detect_hits (orig, hits);
		const samplepos_t pos = file_position (r);
		for (samplepos_t h : hits) {
			guide.push_back (pos + h);
		}
	}
	std::sort (guide.begin (), guide.end ());
	std::vector<samplepos_t> merged;
	for (samplepos_t h : guide) {
		if (merged.empty () || h - merged.back () > 2 * gap) {
			merged.push_back (h);
		}
	}

	/* 2. where each hit should go: the nearest grid line, by `strength` */
	Temporal::TempoMap::SharedPtr tmap = Temporal::TempoMap::use ();
	const int64_t                 gt   = grid.to_ticks ();
	auto target = [&] (samplepos_t h) -> samplepos_t {
		const int64_t t  = tmap->quarters_at (timepos_t (h)).to_ticks ();
		const int64_t rt = (int64_t) llround ((double) t / gt) * gt;
		const samplepos_t g = Temporal::superclock_to_samples (tmap->superclock_at (Temporal::Beats::ticks (rt)), sr);
		return h + (samplepos_t) llround (strength * (g - h));
	};

	/* 3. warp every region with the same timing */
	session.begin_reversible_command (_("quantize audio"));
	std::set<shared_ptr<Playlist> > touched;
	for (size_t n = 0; n < regions.size (); ++n) {
		shared_ptr<AudioRegion> const& r   = regions[n];
		const samplepos_t              pos = file_position (r);
		const samplecnt_t              len = originals[n]->length_samples ();
		Map                            m;
		for (samplepos_t h : merged) {
			if (h <= pos + gap || h >= pos + len - gap) {
				continue;
			}
			m.push_back (Marker { h - pos, target (h) - pos });
		}
		tidy (m, len, gap);
		if (m.empty ()) {
			continue;
		}
		shared_ptr<AudioRegion> nr = render (originals[n], r, m);
		if (nr) {
			replace_in_playlist (r, nr, touched);
		}
	}
	for (shared_ptr<Playlist> const& pl : touched) {
		session.add_command (new StatefulDiffCommand (pl));
	}
	if (touched.empty ()) {
		session.abort_reversible_command ();
	} else {
		session.commit_reversible_command ();
	}
}

void
AudioWarp::move_marker (shared_ptr<AudioRegion> warped, size_t index, samplecnt_t new_out)
{
	Map                     m;
	shared_ptr<AudioRegion> orig = original (warped, m);
	if (orig == warped || index >= m.size ()) {
		return;
	}
	m[index].out = new_out;
	tidy (m, orig->length_samples (), warped->session ().sample_rate () / 200);

	shared_ptr<AudioRegion> nr = render (orig, warped, m);
	if (!nr) {
		return;
	}
	Session& session = warped->session ();
	session.begin_reversible_command (_("move warp marker"));
	std::set<shared_ptr<Playlist> > touched;
	replace_in_playlist (warped, nr, touched);
	for (shared_ptr<Playlist> const& pl : touched) {
		session.add_command (new StatefulDiffCommand (pl));
	}
	session.commit_reversible_command ();
}

bool
AudioWarp::markers (shared_ptr<AudioRegion> r, Map& m)
{
	m.clear ();
	XMLNode* node = r->extra_xml (warp_node_name);
	if (!node) {
		return false;
	}
	for (XMLNode const* c : node->children ()) {
		Marker k;
		if (c->get_property (X_("in"), k.in) && c->get_property (X_("out"), k.out)) {
			m.push_back (k);
		}
	}
	return !m.empty ();
}
