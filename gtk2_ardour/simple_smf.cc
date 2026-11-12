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
#include <cmath>
#include <map>

#include "simple_smf.h"

using std::string;

static void
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

static void
put_be (string& s, uint32_t v, int bytes)
{
	for (int i = bytes - 1; i >= 0; --i) {
		s += (char) ((v >> (8 * i)) & 0xff);
	}
}

string
SimpleSMF::write (std::vector<Note> const& notes, string const& name, double bpm, int num, int den, int64_t end)
{
	struct Ev {
		int64_t t;
		int     order; /* note-offs before note-ons at the same time */
		uint8_t d[3];
	};
	std::vector<Ev> evs;
	for (Note const& n : notes) {
		/* keep every byte in its legal range, whatever we are handed: a
		 * note-on with velocity 0 would read back as a note-off */
		const uint8_t chan  = n.chan & 0x0f;
		const uint8_t pitch = n.pitch & 0x7f;
		const uint8_t vel   = std::max<uint8_t> (1, std::min<uint8_t> (127, n.vel));
		const int64_t on    = std::max<int64_t> (0, n.on);
		evs.push_back ({ on, 1, { (uint8_t) (0x90 | chan), pitch, vel } });
		evs.push_back ({ on + std::max<int64_t> (1, n.len), 0, { (uint8_t) (0x80 | chan), pitch, 0 } });
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
	while (dd < 7 && (1 << dd) < den) {
		++dd;
	}
	trk += '\0';
	trk += "\xff\x58\x04";
	trk += (char) std::max (1, std::min (255, num));
	trk += (char) dd;
	trk += (char) 24;
	trk += (char) 8;
	/* tempo */
	trk += '\0';
	trk += "\xff\x51\x03";
	put_be (trk, (uint32_t) llround (60000000.0 / (bpm > 0 ? bpm : 120.0)), 3);

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

bool
SimpleSMF::read (string const& d, std::vector<Note>& notes)
{
	const uint8_t* p   = (const uint8_t*) d.data ();
	const size_t   len = d.size ();
	if (len < 14 || d.compare (0, 4, "MThd")) {
		return false;
	}
	const int div = (p[12] << 8) | p[13];
	if (div <= 0 || (div & 0x8000)) {
		return false;
	}
	size_t pos = 8 + (((size_t) p[4] << 24) | ((size_t) p[5] << 16) | ((size_t) p[6] << 8) | p[7]);

	while (pos + 8 <= len) {
		const bool   is_track = !d.compare (pos, 4, "MTrk");
		const size_t clen     = ((size_t) p[pos + 4] << 24) | ((size_t) p[pos + 5] << 16) | ((size_t) p[pos + 6] << 8) | p[pos + 7];
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
					Note n;
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
