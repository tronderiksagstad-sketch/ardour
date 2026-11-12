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

namespace PluginKinds {

/** the kinds, in the order the browser lists them: 12 kinds of effect, then 4 of instrument */
enum { FirstEffect = 0, OtherEffect = 11, FirstInstrument = 12, NKinds = 16 };

std::string              lower (std::string const& s);  ///< lower case (UTF-8)
std::string              squash (std::string const& s); ///< lower case letters and digits only: "A-Comp 2" -> "acomp2"
std::vector<std::string> tokens (std::string const& s); ///< the lower-case words of s

/** which kind a plugin is, from what it says about itself (its category, its name, its tags) */
int guess_kind (std::string const& text, bool instrument, bool analyzer);

/** what a search word is held against, for one plugin */
struct Subject {
	std::string const& lname;       ///< name, lower case
	std::string const& lmaker;      ///< maker, lower case
	std::string const& tags;        ///< lower case, space separated
	std::string        description; ///< the line under the name
	bool               own_note;    ///< that line is the user's own note
	std::string        kind_label;  ///< the kind's name as shown
	const char*        kind_words;  ///< other words for the kind
};

struct Fit {
	int best;      ///< 0: the word is nowhere; higher is a better place (100: the name starts with it)
	int hit_start; ///< where in the name the word was found, or -1
	int hit_len;
};

/** how well one (lower-case) search word fits a plugin */
Fit fit (Subject const& s, std::string const& word);

}
