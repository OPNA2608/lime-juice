//
// lime-juice: C++ port of Tomyun's "Juice" de/recompiler for PC-98 games
// Copyright (C) 2026 Fuzion
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#pragma once

#include "image/image.h"

#include <cstddef>
#include <cstdint>
#include <vector>

// shared GPC compression primitives used by both GPC and GPA codecs

namespace gpc {
namespace internal {

// decode compressed frame data into pixel indices.
// reads from data starting at pos (pos is updated).
// w, h: frame pixel dimensions.
// interleaving: vertical interlace value.
// returns w*h pixel indices (0-15), row-major.
std::vector<uint8_t> decode_frame_data(const std::vector<uint8_t>& data,
                                       size_t& pos, int w, int h,
                                       int interleaving);

struct CompressedFrame {
    std::vector<uint8_t> data;
    int interleaving;
};

// encode pixel data into compressed frame bytes.
// pixels: w*h palette indices (0-15), row-major.
// returns compressed data and chosen interleave value.
CompressedFrame encode_frame_data(const std::vector<uint8_t>& pixels,
                                  int w, int h);

} // namespace internal
} // namespace gpc
