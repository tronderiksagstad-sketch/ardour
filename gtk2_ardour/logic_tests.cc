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

/* Tests for the parts of this fork that are pure logic: the Ctrl+Space
 * search matching, the plugin browser's kind guessing and search ranking,
 * the MIDI file writer/reader used for MuseScore, and the warp-marker rule.
 *
 * Not part of the program. Build and run with:   tools/logic-tests.sh
 * (a few seconds; needs only g++ and glib, not an Ardour build).
 * The script builds this with the memory and undefined-behaviour checkers
 * on, so reading outside a buffer fails the run even when the answer
 * happens to come out right.
 */

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include <glib.h>

#include "plugin_kinds.h"
#include "simple_smf.h"
#include "text_match.h"
#include "warp_map.h"

using std::string;
using std::vector;

static int checks   = 0;
static int failures = 0;

#define CHECK(cond)                                                              \
	do {                                                                     \
		++checks;                                                        \
		if (!(cond)) {                                                   \
			++failures;                                              \
			printf ("  FAILED  %s:%d  %s\n", __FILE__, __LINE__, #cond); \
		}                                                                \
	} while (0)

#define CHECK_EQ(a, b)                                                                                                 \
	do {                                                                                                           \
		++checks;                                                                                              \
		const long long va = (long long) (a), vb = (long long) (b);                                            \
		if (va != vb) {                                                                                        \
			++failures;                                                                                    \
			printf ("  FAILED  %s:%d  %s == %s  (%lld, %lld)\n", __FILE__, __LINE__, #a, #b, va, vb);       \
		}                                                                                                      \
	} while (0)

static std::mt19937 rng (20261112);

static string
random_bytes (size_t max_len)
{
	string s;
	const size_t n = rng () % (max_len + 1);
	for (size_t i = 0; i < n; ++i) {
		/* mostly letters, some spaces, and a good share of bytes that break UTF-8 */
		const unsigned r = rng () % 10;
		s += (char) (r < 6 ? 'a' + rng () % 26 : r < 7 ? ' ' : rng () % 256);
	}
	return s;
}

/* ---- Ctrl+Space search ------------------------------------------------ */

static int
score_of (string const& text, string const& query)
{
	vector<size_t> hits;
	int            score = 0;
	return TextMatch::match (text, query, hits, score) ? score : -1;
}

static void
test_text_match ()
{
	printf ("search matching\n");
	vector<size_t> hits;
	int            score;

	/* a whole word, wherever it is, with the letters it matched */
	CHECK (TextMatch::match ("Add Mono Track", "mono", hits, score));
	CHECK_EQ (hits.size (), 4);
	CHECK_EQ (hits.front (), 4);
	CHECK_EQ (hits.back (), 7);

	/* start of the text beats start of a word beats the middle of a word */
	CHECK (score_of ("Snapshot", "sna") > score_of ("Take Snapshot", "sna"));
	CHECK (score_of ("Take Snapshot", "sna") > score_of ("Unsnapped", "sna"));

	/* letters in order but not together still match, below any whole match */
	CHECK (score_of ("Toggle Record Enable", "tre") > 0);
	CHECK (score_of ("Toggle Record Enable", "tre") < score_of ("Unsnapped", "sna"));

	/* ... unless they are far apart, or not there */
	CHECK_EQ (score_of ("a.............................b", "ab"), -1);
	CHECK_EQ (score_of ("Add Mono Track", "zzz"), -1);
	CHECK_EQ (score_of ("Add Mono Track", ""), -1);
	CHECK_EQ (score_of ("", "a"), -1);

	/* capitals do not matter, for Norwegian letters either */
	CHECK (score_of ("BL\xC3\x85S", "bl\xC3\xA5s") > 0);   /* "BLÅS" holds "blås" */
	CHECK (score_of ("bl\xC3\xA5sere", "\xC3\x85S") > 0);  /* "blåsere" holds "ÅS" */

	/* hits count letters, not bytes: "Kor æøå test" has "test" at letter 8 */
	CHECK (TextMatch::match ("Kor \xC3\xA6\xC3\xB8\xC3\xA5 test", "test", hits, score));
	CHECK_EQ (hits.front (), 8);

	/* a letter-by-letter match never lands inside a letter */
	CHECK (TextMatch::match ("r\xC3\xB8""d bl\xC3\xA5", "\xC3\xB8\xC3\xA5", hits, score)); /* "ø" and "å" out of "rød blå" */
	CHECK_EQ (hits.size (), 2);
	CHECK_EQ (hits[0], 1);
	CHECK_EQ (hits[1], 6);

	/* text that is not proper UTF-8 is searched all the same, and what comes back is fit for display */
	CHECK (score_of ("\xff\xfe abc \xc3", "abc") > 0);
	vector<string> pieces;
	TextMatch::split_chars ("ok \xff\xc2 \xc3", pieces);
	string joined;
	for (string const& p : pieces) {
		joined += p;
	}
	CHECK (g_utf8_validate (joined.data (), joined.size (), 0));
	CHECK_EQ (pieces.size (), 7);

	/* anything at all: no crash, hits inside the text, pieces valid */
	for (int i = 0; i < 4000; ++i) {
		const string text  = random_bytes (40);
		const string query = random_bytes (6);
		vector<gunichar> lower;
		TextMatch::split_chars (text, pieces, &lower);
		CHECK_EQ (pieces.size (), lower.size ());
		joined.clear ();
		for (string const& p : pieces) {
			joined += p;
		}
		if (!g_utf8_validate (joined.data (), joined.size (), 0)) {
			CHECK (!"split_chars gave invalid text");
		}
		if (TextMatch::match (text, query, hits, score)) {
			if (hits.empty () || hits.back () >= pieces.size () || !std::is_sorted (hits.begin (), hits.end ())) {
				CHECK (!"hits out of range");
			}
		}
	}
}

/* ---- plugin browser ---------------------------------------------------- */

static int
fit_of (string const& name, string const& word, string const& maker = "", string const& tags = "", string const& description = "",
        bool own_note = false, string const& kind_label = "", const char* kind_words = "")
{
	const string lname = PluginKinds::lower (name), lmaker = PluginKinds::lower (maker);
	PluginKinds::Subject s = { lname, lmaker, tags, description, own_note, kind_label, kind_words };
	return PluginKinds::fit (s, word).best;
}

static void
test_plugin_kinds ()
{
	using namespace PluginKinds;
	printf ("plugin kinds and search\n");

	/* words */
	const vector<string> t = tokens ("A-Comp 2 (Stereo)");
	CHECK_EQ (t.size (), 4);
	CHECK (t.size () == 4 && t[0] == "a" && t[1] == "comp" && t[2] == "2" && t[3] == "stereo");
	CHECK (squash ("A-Comp 2") == "acomp2");
	CHECK (lower ("\xC3\x85RE") == "\xC3\xA5re");        /* "ÅRE" -> "åre" */
	CHECK (lower ("R\xF6yk SOPP\xC3") == "r\xF6yk sopp\xC3"); /* not UTF-8: left alone but for a-z, and no reading past the end */
	CHECK (tokens ("").empty ());

	/* kinds of effect, from category + name as real plugins give them */
	enum { EQ = 0, Comp, Limit, Gate, Reverb, Delay, Dist, Mod, Pitch, Tools, Meter, Other };
	struct { const char* text; int kind; bool analyzer; } effects[] = {
		{ "EQ a-EQ", EQ, false },
		{ "Filter a-High/Low Pass Filter", EQ, false },
		{ "Compressor a-Compressor", Comp, false },
		{ "Dynamics LSP Dynamikprozessor", Comp, false },
		{ "Limiter x42-dpl - Digital Peak Limiter", Limit, false },
		{ "Expander a-Expander", Gate, false },
		{ "Dynamics Noise Gate", Gate, false },
		{ "Reverb a-Reverb", Reverb, false },
		{ "Plugin Dragonfly Hall Reverb", Reverb, false },
		{ "Delay a-Delay", Delay, false },
		{ "Plugin Tape Echo", Delay, false },
		{ "Distortion Guitarix Amp", Dist, false },
		{ "Plugin Overdrive Pedal", Dist, false },
		{ "Chorus Calf Multi Chorus", Mod, false },
		{ "Plugin Rotary Speaker", Mod, false },
		{ "Pitch Shifter x42-Autotune", Pitch, false },
		{ "Utility a-Amplifier", Tools, false },
		{ "Utility ACE Mute", Tools, false },
		{ "Analyser x42 Spectrum", Meter, false },
		{ "Plugin Something Odd", Meter, true }, /* says it is an analyser */
		{ "Plugin Mystery Box", Other, false },
		{ "", Other, false },
	};
	for (auto const& e : effects) {
		const int k = guess_kind (e.text, false, e.analyzer);
		if (k != e.kind) {
			printf ("    \"%s\" filed as %d, expected %d\n", e.text, k, e.kind);
		}
		CHECK_EQ (k, e.kind);
	}

	/* kinds of instrument */
	enum { Synth = FirstInstrument, Keys, Drums, Sampler };
	struct { const char* text; int kind; } instruments[] = {
		{ "Instrument Vital", Synth },
		{ "Instrument General MIDI Synth", Synth },
		{ "Instrument setBfree DSP Tonewheel Organ", Keys },
		{ "Instrument Pianoteq piano", Keys },
		{ "Instrument DrumGizmo drum kit", Drums },
		{ "Instrument Geonkick", Drums },
		{ "Instrument ACE Fluid Synth", Sampler },
		{ "Instrument sfizz", Sampler },
		{ "", Synth },
	};
	for (auto const& e : instruments) {
		const int k = guess_kind (e.text, true, false);
		if (k != e.kind) {
			printf ("    \"%s\" filed as %d, expected %d\n", e.text, k, e.kind);
		}
		CHECK_EQ (k, e.kind);
	}
	for (int i = 0; i < 2000; ++i) {
		const int k = guess_kind (random_bytes (40), rng () % 2, rng () % 2);
		if (k < 0 || k >= NKinds) {
			CHECK (!"kind out of range");
		}
	}

	/* where a search word is found decides how high the plugin comes */
	CHECK_EQ (fit_of ("a-Comp", "a"), 100);                 /* the name starts with it */
	CHECK_EQ (fit_of ("ACE Reverb", "rev"), 80);            /* a word of the name starts with it */
	CHECK_EQ (fit_of ("a-Comp", "acomp"), 70);              /* the name, written without its signs */
	CHECK_EQ (fit_of ("Thing", "vocal", "", "vocal warm"), 65);            /* a tag */
	CHECK_EQ (fit_of ("Supermassive", "mass"), 50);         /* the middle of a word of the name */
	CHECK_EQ (fit_of ("Thing", "val", "Valhalla DSP"), 45);                 /* the maker */
	CHECK_EQ (fit_of ("Thing", "bright", "", "", "My bright one", true), 42);   /* your own note */
	CHECK_EQ (fit_of ("Thing", "warm", "", "", "Adds warm grit."), 34);    /* our description */
	CHECK_EQ (fit_of ("Thing", "verb", "", "", "", false, "Reverb", "reverb verb room"), 30); /* its kind */
	CHECK_EQ (fit_of ("Thing", "roo", "", "", "", false, "Reverb", "reverb verb room"), 30);
	CHECK_EQ (fit_of ("Thing", "zzz", "Maker", "tag", "Words here", false, "Reverb", "reverb"), 0);
	CHECK_EQ (fit_of ("Thing", "roo", "", "", "", false, "Reverb", 0), 0);  /* a kind without other words */

	/* the letters to light up are those of the name */
	const string lname = "ace reverb", none;
	Subject      s     = { lname, none, none, "", false, "", "" };
	const Fit    f     = fit (s, "rev");
	CHECK_EQ (f.hit_start, 4);
	CHECK_EQ (f.hit_len, 3);

	for (int i = 0; i < 2000; ++i) {
		const string n = lower (random_bytes (30)), w = lower (random_bytes (5));
		if (w.empty ()) {
			continue;
		}
		Subject   r  = { n, none, none, random_bytes (30), false, random_bytes (10), "eq filter" };
		const Fit rf = fit (r, w);
		if (rf.best < 0 || rf.best > 100 || (rf.hit_start >= 0 && (size_t) (rf.hit_start + rf.hit_len) > n.size ())) {
			CHECK (!"fit out of range");
		}
	}
}

/* ---- MIDI files for MuseScore ------------------------------------------ */

static bool
same_notes (vector<SimpleSMF::Note> a, vector<SimpleSMF::Note> b)
{
	auto order = [] (SimpleSMF::Note const& x, SimpleSMF::Note const& y) {
		return x.on != y.on ? x.on < y.on : x.pitch != y.pitch ? x.pitch < y.pitch : x.chan < y.chan;
	};
	std::sort (a.begin (), a.end (), order);
	std::sort (b.begin (), b.end (), order);
	if (a.size () != b.size ()) {
		return false;
	}
	for (size_t i = 0; i < a.size (); ++i) {
		if (a[i].on != b[i].on || a[i].len != b[i].len || a[i].chan != b[i].chan || a[i].pitch != b[i].pitch || a[i].vel != b[i].vel) {
			return false;
		}
	}
	return true;
}

static void
test_smf ()
{
	using namespace SimpleSMF;
	printf ("MIDI files\n");

	/* what goes out comes back the same */
	vector<Note> notes = {
		{ 0, 480, 0, 60, 100 }, { 0, 240, 0, 64, 90 }, { 480, 480, 0, 62, 80 }, { 960, 1920, 3, 36, 127 }, { 100000, 7, 15, 127, 1 },
	};
	const string file = write (notes, "Test \xC3\xA6\xC3\xB8\xC3\xA5", 97.5, 6, 8, 200000);
	vector<Note> back;
	CHECK (read (file, back));
	CHECK (same_notes (notes, back));

	/* the file is what other programs expect: header, one track, our resolution */
	CHECK (file.compare (0, 4, "MThd") == 0);
	CHECK_EQ ((unsigned char) file[9], 0);  /* format 0 */
	CHECK_EQ ((unsigned char) file[11], 1); /* one track */
	CHECK_EQ (((unsigned char) file[12] << 8) | (unsigned char) file[13], ppqn);
	CHECK (file.compare (14, 4, "MTrk") == 0);
	const size_t track_len = ((size_t) (unsigned char) file[18] << 24) | ((size_t) (unsigned char) file[19] << 16) | ((size_t) (unsigned char) file[20] << 8) | (unsigned char) file[21];
	CHECK_EQ (track_len + 22, file.size ());
	CHECK (file.compare (file.size () - 3, 3, string ("\xff\x2f\x00", 3)) == 0); /* ends with end-of-track */

	/* odd input is made legal rather than written out wrong */
	vector<Note> odd = { { 0, 0, 0, 60, 0 }, { -50, 10, 200, 61, 255 } };
	back.clear ();
	CHECK (read (write (odd, "", 0, 0, 0, -1), back));
	CHECK_EQ (back.size (), 2);
	for (Note const& n : back) {
		CHECK (n.len >= 1 && n.on >= 0 && n.chan <= 15 && n.pitch <= 127 && n.vel >= 1 && n.vel <= 127);
	}

	/* a file at another resolution is scaled to ours */
	string fine = file;
	fine[12]    = (char) (960 >> 8);
	fine[13]    = (char) (960 & 0xff);
	back.clear ();
	CHECK (read (fine, back));
	CHECK_EQ (back.size (), notes.size ());
	std::sort (back.begin (), back.end (), [] (Note const& x, Note const& y) { return x.on != y.on ? x.on < y.on : x.pitch < y.pitch; });
	CHECK_EQ (back[2].on, 240);
	CHECK_EQ (back[2].len, 240);

	/* what MuseScore writes: two tracks, running status, note-on with velocity 0 as note-off,
	 * and events we do not care about in between */
	const unsigned char raw[] = {
		'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1, 0, 2, 0x01, 0xE0,
		'M', 'T', 'r', 'k', 0, 0, 0, 22,
		0x00, 0xFF, 0x51, 0x03, 0x07, 0xA1, 0x20, /* tempo */
		0x00, 0xC0, 0x05,                         /* program change */
		0x00, 0x90, 60, 100,                      /* note on */
		0x83, 0x60, 60, 0,                        /* 480 ticks later, running status, velocity 0 */
		0x00, 0xFF, 0x2F, 0x00,
		'M', 'T', 'r', 'k', 0, 0, 0, 22,
		0x00, 0xB1, 7, 100,                       /* a controller */
		0x00, 0x91, 40, 70,
		0x81, 0x70, 0x81, 40, 64,                 /* 240 ticks later, a real note-off */
		0x00, 0xF0, 0x02, 0x7E, 0xF7,             /* sysex */
		0x00, 0xFF, 0x2F, 0x00,
	};
	back.clear ();
	CHECK (read (string ((const char*) raw, sizeof (raw)), back));
	CHECK_EQ (back.size (), 2);
	if (back.size () == 2) {
		CHECK (back[0].on == 0 && back[0].len == 480 && back[0].chan == 0 && back[0].pitch == 60 && back[0].vel == 100);
		CHECK (back[1].on == 0 && back[1].len == 240 && back[1].chan == 1 && back[1].pitch == 40 && back[1].vel == 70);
	}

	/* not MIDI files */
	back.clear ();
	CHECK (!read ("", back));
	CHECK (!read ("MThd", back));
	CHECK (!read ("RIFF" + file.substr (4), back));
	string smpte = file;
	smpte[12]    = (char) 0xE7; /* time in frames, not beats */
	CHECK (!read (smpte, back));
	string zero = file;
	zero[12] = zero[13] = 0;
	CHECK (!read (zero, back));
	CHECK (back.empty ());

	/* a file cut short anywhere, or damaged anywhere: never a read outside it */
	for (size_t n = 0; n <= file.size (); ++n) {
		back.clear ();
		read (file.substr (0, n), back);
	}
	for (int i = 0; i < 30000; ++i) {
		string bad = (i % 2) ? file : string ((const char*) raw, sizeof (raw));
		const int changes = 1 + rng () % 4;
		for (int c = 0; c < changes; ++c) {
			bad[rng () % bad.size ()] = (char) (rng () % 256);
		}
		if (rng () % 4 == 0) {
			bad.resize (rng () % (bad.size () + 1));
		}
		back.clear ();
		read (bad, back);
		for (Note const& n : back) {
			if (n.len < 1 || n.on < 0 || n.chan > 15) {
				CHECK (!"a damaged file gave an impossible note");
			}
		}
	}
	++checks;
}

/* ---- warp markers ------------------------------------------------------- */

static void
test_warp_map ()
{
	using namespace AudioWarp;
	printf ("warp markers\n");

	const int64_t length = 100000, gap = 220;

	/* put in order */
	Map m = { { 50000, 50100 }, { 10000, 9000 }, { 30000, 31000 } };
	tidy (m, length, gap);
	CHECK_EQ (m.size (), 3);
	CHECK (m.size () == 3 && m[0].in == 10000 && m[1].in == 30000 && m[2].in == 50000);

	/* two hits too close together in the original: the later one goes */
	m = { { 10000, 10000 }, { 10100, 12000 }, { 20000, 20000 } };
	tidy (m, length, gap);
	CHECK_EQ (m.size (), 2);
	CHECK (m.size () == 2 && m[1].in == 20000);

	/* a marker that would play before the one in front of it: it goes */
	m = { { 10000, 30000 }, { 20000, 25000 }, { 40000, 40000 } };
	tidy (m, length, gap);
	CHECK_EQ (m.size (), 2);
	CHECK (m.size () == 2 && m[0].in == 10000 && m[1].in == 40000);

	/* nothing right at the start or the end of the region */
	m = { { 100, 100 }, { 50000, 50000 }, { 99900, 99900 }, { 60000, 99990 } };
	tidy (m, length, gap);
	CHECK_EQ (m.size (), 1);

	/* nothing to do */
	m.clear ();
	tidy (m, length, gap);
	CHECK (m.empty ());
	tidy (m, 0, gap);
	CHECK (m.empty ());

	/* whatever goes in: what comes out is in order, apart, inside the region, and tidying again changes nothing */
	for (int i = 0; i < 3000; ++i) {
		Map r;
		const int n = rng () % 40;
		for (int k = 0; k < n; ++k) {
			r.push_back ({ (int64_t) (rng () % 110000) - 5000, (int64_t) (rng () % 110000) - 5000 });
		}
		tidy (r, length, gap);
		bool ok = true;
		for (size_t k = 0; k < r.size (); ++k) {
			ok = ok && r[k].in >= gap && r[k].out >= gap && r[k].in <= length - gap && r[k].out <= length - gap;
			if (k > 0) {
				ok = ok && r[k].in - r[k - 1].in >= gap && r[k].out - r[k - 1].out >= gap;
			}
		}
		Map again = r;
		tidy (again, length, gap);
		ok = ok && again.size () == r.size ();
		if (!ok) {
			CHECK (!"tidy left markers out of order, too close or outside the region");
		}
	}
	++checks;
}

int
main ()
{
	test_text_match ();
	test_plugin_kinds ();
	test_smf ();
	test_warp_map ();
	if (failures) {
		printf ("\n%d of %d checks FAILED\n", failures, checks);
		return 1;
	}
	printf ("\nall %d checks passed\n", checks);
	return 0;
}
