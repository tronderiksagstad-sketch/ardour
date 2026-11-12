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

#include "warp_map.h"

void
AudioWarp::tidy (Map& m, int64_t length, int64_t min_gap)
{
	std::sort (m.begin (), m.end (), [] (Marker const& a, Marker const& b) { return a.in < b.in; });
	Map     out;
	int64_t last_in = 0, last_out = 0;
	for (Marker const& k : m) {
		if (k.in < last_in + min_gap || k.out < last_out + min_gap) {
			continue;
		}
		if (k.in > length - min_gap || k.out > length - min_gap) {
			continue;
		}
		out.push_back (k);
		last_in  = k.in;
		last_out = k.out;
	}
	m.swap (out);
}
