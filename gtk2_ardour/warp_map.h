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
#include <vector>

/* The list of warp markers of a quantized audio region, and the rule that
 * keeps it sane. Nothing here needs the rest of Ardour, so it can be
 * tested on its own (see logic_tests.cc). */
namespace AudioWarp {

/** one warp marker: where the hit is in the original audio, and where it
 * should play, both in samples from the start of the region */
struct Marker {
	int64_t in;
	int64_t out;
};

typedef std::vector<Marker> Map;

/** keep markers in order and apart, inside the region; drops ones that would cross */
void tidy (Map& m, int64_t length, int64_t min_gap);

}
