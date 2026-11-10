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

#include <memory>
#include <utility>
#include <vector>

#include "temporal/beats.h"

#include "ardour/types.h"

namespace ARDOUR {
	class AudioRegion;
}

/** Audio warp: stretch audio between "hits" (warp markers) so that each hit
 * lands where we want it, without cutting the audio.
 *
 * A warped region remembers the region it was made from (its sources,
 * start and length) and the list of markers, in its extra XML. Every new
 * warp - quantizing again, or dragging a marker by hand - is rendered from
 * that original, so the sound never degrades through repeated stretching.
 */
namespace AudioWarp {

/** one warp marker: where the hit is in the original audio, and where it
 * should play, both in samples from the start of the region */
struct Marker {
	ARDOUR::samplecnt_t in;
	ARDOUR::samplecnt_t out;
};

typedef std::vector<Marker> Map;

/** the unwarped original of `r` (r itself if it was never warped),
 * and r's current markers (empty if never warped) */
std::shared_ptr<ARDOUR::AudioRegion> original (std::shared_ptr<ARDOUR::AudioRegion> r, Map& markers);

/** the markers stored in a warped region, without rebuilding its original */
bool markers (std::shared_ptr<ARDOUR::AudioRegion> r, Map& m);

/** find the hits (drum hits, note starts) in a region, in samples from its start */
void detect_hits (std::shared_ptr<ARDOUR::AudioRegion> r, std::vector<ARDOUR::samplepos_t>& hits);

/** keep markers in order and apart, inside the region; drops ones that would cross */
void tidy (Map& m, ARDOUR::samplecnt_t length, ARDOUR::samplecnt_t min_gap);

/** Quantize: find the hits in all `regions` together, move them toward the
 * nearest multiple of `grid` (strength 0..1), and replace each region with
 * its warped version (one undo step). All regions are warped with the same
 * timing, so recordings of the same performance stay in sync. */
void quantize (std::vector<std::shared_ptr<ARDOUR::AudioRegion> > const& regions, Temporal::Beats grid, double strength);

/** move one marker of a warped region by hand (one undo step) */
void move_marker (std::shared_ptr<ARDOUR::AudioRegion> warped, size_t index, ARDOUR::samplecnt_t new_out);

/** render `orig` warped by `markers` into new audio files; the new region
 * has the position and length of `placed_like` and remembers orig and the markers.
 * Returns null on failure. */
std::shared_ptr<ARDOUR::AudioRegion> render (std::shared_ptr<ARDOUR::AudioRegion> orig,
                                             std::shared_ptr<ARDOUR::AudioRegion> placed_like,
                                             Map const& markers);

}
