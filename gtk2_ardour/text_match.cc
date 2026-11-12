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


/* Finding typed text inside names: used by the Ctrl+Space search.
 * Nothing here needs the rest of Ardour, so it can be tested on its own
 * (see logic_tests.cc). */

#include <algorithm>

#include "text_match.h"

/* Text as a row of characters, so that names with accented or other
 * non-English letters are searched and shown one letter at a time rather
 * than one byte at a time. Bytes that are not proper UTF-8 become a
 * replacement mark, which keeps the result fit for display. */
void
TextMatch::split_chars (std::string const& s, std::vector<std::string>& pieces, std::vector<gunichar>* lower)
{
	pieces.clear ();
	if (lower) {
		lower->clear ();
	}
	const char* p   = s.data ();
	const char* end = p + s.size ();
	while (p < end) {
		const gunichar c = g_utf8_get_char_validated (p, end - p);
		if (c == (gunichar) -1 || c == (gunichar) -2 || c == 0) {
			pieces.push_back ("\xEF\xBF\xBD");
			if (lower) {
				lower->push_back (0xFFFD);
			}
			++p;
			continue;
		}
		const char* next = g_utf8_next_char (p);
		pieces.push_back (std::string (p, next - p));
		if (lower) {
			lower->push_back (g_unichar_tolower (c));
		}
		p = next;
	}
}

/* Find the query in the text (case-insensitive). The whole query as one
 * piece scores highest, more so at the start of the text or of a word.
 * Otherwise its letters must appear in order, close together; matches
 * whose letters are far apart are not shown.
 * hits are the positions of the matched letters, counted in characters. */
bool
TextMatch::match (std::string const& text, std::string const& query, std::vector<size_t>& hits, int& score)
{
	hits.clear ();
	score = 0;

	std::vector<std::string> unused;
	std::vector<gunichar>    lt, lq;
	split_chars (text, unused, &lt);
	split_chars (query, unused, &lq);
	if (lq.empty ()) {
		return false;
	}

	std::vector<gunichar>::const_iterator at = std::search (lt.begin (), lt.end (), lq.begin (), lq.end ());
	if (at != lt.end ()) {
		const size_t pos = at - lt.begin ();
		for (size_t i = 0; i < lq.size (); ++i) {
			hits.push_back (pos + i);
		}
		score = 1000 - (int) pos - (int) lt.size () / 4;
		if (pos == 0) {
			score += 300;
		} else if (!g_unichar_isalnum (lt[pos - 1])) {
			score += 200;
		}
		return true;
	}

	size_t from = 0;
	int    gaps = 0;
	int    starts = 0;
	for (gunichar q : lq) {
		if (q == ' ') {
			continue;
		}
		std::vector<gunichar>::const_iterator f = std::find (lt.begin () + from, lt.end (), q);
		if (f == lt.end ()) {
			return false;
		}
		const size_t i = f - lt.begin ();
		if (!hits.empty ()) {
			gaps += (int) (i - hits.back () - 1);
		}
		if (i == 0 || !g_unichar_isalnum (lt[i - 1])) {
			++starts;
		}
		hits.push_back (i);
		from = i + 1;
	}
	if (hits.empty () || gaps > 3 + 2 * (int) lq.size ()) {
		return false;
	}
	score = 400 + 40 * starts - 15 * gaps - (int) hits.front () / 2;
	return true;
}
