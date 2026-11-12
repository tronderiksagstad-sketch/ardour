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


/* Which kind a plugin is, and how well it answers a search word: the
 * thinking part of the plugin browser. Nothing here needs the rest of
 * Ardour, so it can be tested on its own (see logic_tests.cc). */

#include <cstring>
#include <initializer_list>

#include <glib.h>

#include "plugin_kinds.h"

using std::string;
using std::vector;

string
PluginKinds::lower (string const& s)
{
	/* text that is not proper UTF-8 must not go through the UTF-8 routines */
	char*  l = g_utf8_validate (s.data (), s.size (), 0) ? g_utf8_strdown (s.data (), s.size ()) : g_ascii_strdown (s.data (), s.size ());
	string r (l);
	g_free (l);
	return r;
}

string
PluginKinds::squash (string const& s)
{
	string out;
	for (char c : s) {
		if (g_ascii_isalnum (c)) {
			out += g_ascii_tolower (c);
		}
	}
	return out;
}

vector<string>
PluginKinds::tokens (string const& s)
{
	vector<string> out;
	string         cur;
	for (char c : lower (s)) {
		if (g_ascii_isalnum (c) || (unsigned char) c >= 0x80) {
			cur += c;
		} else if (!cur.empty ()) {
			out.push_back (cur);
			cur.clear ();
		}
	}
	if (!cur.empty ()) {
		out.push_back (cur);
	}
	return out;
}

/* "=x": a word that is exactly x; "*x": a word containing x; else a word beginning with x */
static bool
has_word (vector<string> const& t, const char* kw)
{
	for (string const& w : t) {
		if (kw[0] == '=') {
			if (w == kw + 1) { return true; }
		} else if (kw[0] == '*') {
			if (w.find (kw + 1) != string::npos) { return true; }
		} else if (w.compare (0, strlen (kw), kw) == 0) {
			return true;
		}
	}
	return false;
}

static bool
has_any (vector<string> const& t, std::initializer_list<const char*> kws)
{
	for (const char* k : kws) {
		if (has_word (t, k)) {
			return true;
		}
	}
	return false;
}

/** which kind a plugin is, from what it says about itself (its category, its name, its tags) */
int
PluginKinds::guess_kind (string const& text, bool instrument, bool analyzer)
{
	vector<string> t = tokens (text);

	if (instrument) {
		if (has_any (t, { "drum", "kit", "percuss", "=beat", "geonkick", "sitala", "=808", "=909" })) { return FirstInstrument + 2; }
		if (has_any (t, { "sampl", "soundfont", "=sfz", "sfizz", "=sf2", "fluid", "kontakt" })) { return FirstInstrument + 3; }
		if (has_any (t, { "piano", "=keys", "organ", "keyboard", "epiano", "rhodes", "wurli", "clav", "harpsi", "tonewheel", "b3" })) { return FirstInstrument + 1; }
		return FirstInstrument;
	}
	if (analyzer || has_any (t, { "analy", "=meter", "*meter", "spectr", "scope", "=vu", "oscillo" })) { return 10; }
	if (has_any (t, { "limit", "maximi", "loudmax", "clipper" })) { return 2; }
	if (has_any (t, { "gate", "expand", "denois" })) { return 3; }
	if (has_any (t, { "compress", "=comp", "dynamic", "squash", "leveler", "levelling" })) { return 1; }
	if (has_any (t, { "*reverb", "=verb", "=room", "=hall", "=plate", "=spring", "ambien", "reflection", "convolv" })) { return 4; }
	if (has_any (t, { "delay", "echo", "=dub" })) { return 5; }
	if (has_any (t, { "pitch", "=tune", "tuning", "tuner", "autotune", "harmoniz", "=graillon" })) { return 8; }
	if (has_any (t, { "distort", "overdrive", "fuzz", "=amp", "saturat", "=tape", "guitar", "=drive", "crush", "shaper", "waveshap", "=nam", "=cab", "cabinet", "preamp", "exciter" })) { return 6; }
	if (has_any (t, { "chorus", "flang", "=phaser", "tremolo", "vibrato", "ensemble", "rotary", "leslie", "modulat", "=wah", "vocod" })) { return 7; }
	if (has_any (t, { "=eq", "equali", "filter", "shelf", "highpass", "lowpass", "=hpf", "=lpf", "bandpass", "crossover", "=tone", "=bass", "=treble" })) { return 0; }
	if (has_any (t, { "stereo", "width", "balance", "=phase", "utilit", "amplif", "crossfade", "=cross", "=gain", "=trim", "=mono", "=pan", "panner", "=ms", "=mixer", "invert", "polarity", "rotate", "=switch", "=mute", "=route", "=routing", "=a/b" })) { return 9; }
	return OtherEffect;
}

PluginKinds::Fit
PluginKinds::fit (Subject const& e, string const& t)
{
	Fit f = { 0, -1, 0 };
	const string& name = e.lname;
	const size_t  i    = name.find (t);
	if (i == 0) {
		f = { 100, 0, (int) t.size () };
	} else if (i != string::npos && !g_ascii_isalnum (name[i - 1])) {
		f = { 80, (int) i, (int) t.size () };
	} else if (squash (t).size () > 2 && i == string::npos && squash (name).find (squash (t)) != string::npos) {
		f.best = 70;
	} else if (i != string::npos) {
		f = { 50, (int) i, (int) t.size () };
	}
	if (f.best < 65) {
		for (string const& w : tokens (e.tags)) {
			if (w == t) {
				f.best = 65;
				break;
			}
		}
	}
	const size_t m = e.lmaker.find (t);
	if (f.best < 45 && m != string::npos && (m == 0 || e.lmaker[m - 1] == ' ')) {
		f.best = 45;
	}
	if (f.best < 40 && t.size () > 2 && e.tags.find (t) != string::npos) {
		f.best = 40;
	}
	if (f.best < 42 && t.size () > 2) {
		const string d = " " + lower (e.description);
		if (d.find (" " + t) != string::npos) {
			f.best = e.own_note ? 42 : 34;
		}
	}
	if (f.best < 30 && t.size () > 1) {
		if (lower (e.kind_label).find (t) != string::npos) {
			f.best = 30;
		} else {
			for (string const& w : tokens (e.kind_words ? e.kind_words : "")) {
				if (w.compare (0, t.size (), t) == 0) {
					f.best = 30;
					break;
				}
			}
		}
	}
	return f;
}
