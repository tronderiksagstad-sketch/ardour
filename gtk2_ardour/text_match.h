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


#pragma once

#include <string>
#include <vector>

#include <glib.h>

namespace TextMatch {

/** Text as a row of characters (not bytes), with the lower-case form of
 * each if asked for. Bytes that are not proper UTF-8 become U+FFFD. */
void split_chars (std::string const& s, std::vector<std::string>& pieces, std::vector<gunichar>* lower = 0);

/** Find `query` in `text`, ignoring case. On success `hits` holds the
 * positions of the matched letters, counted in characters, and `score`
 * says how good the match is (higher is better). */
bool match (std::string const& text, std::string const& query, std::vector<size_t>& hits, int& score);

}
