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

#include <cstdint>
#include <string>
#include <vector>

/** A minimal Standard MIDI File writer and reader: notes only, which is
 * all that "Open in MuseScore" passes back and forth. Nothing here needs
 * the rest of Ardour, so it can be tested on its own (see logic_tests.cc). */
namespace SimpleSMF {

const int ppqn = 480; ///< ticks per quarter note in what we write, and in what read() returns

struct Note {
	int64_t on;  ///< ticks from the start of the file
	int64_t len; ///< ticks
	uint8_t chan, pitch, vel;
};

/** a one-track MIDI file holding `notes`, `end` ticks long */
std::string write (std::vector<Note> const& notes, std::string const& name, double bpm, int num, int den, int64_t end);

/** the notes of all tracks of a MIDI file given as bytes; times are scaled to our ppqn.
 * Returns false if this is not a MIDI file we can read. Never reads outside `bytes`. */
bool read (std::string const& bytes, std::vector<Note>& notes);

}
