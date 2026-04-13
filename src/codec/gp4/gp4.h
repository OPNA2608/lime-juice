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

#include <cstdint>
#include <vector>

namespace gp4 {

// decode a GP4 file from raw bytes into an IndexedImage.
// canvas_w overrides the internal canvas width (default 640).
// some games use a wider virtual framebuffer for scrolling scenes.
IndexedImage decode(const std::vector<uint8_t>& data, int canvas_w = 640);

// encode an IndexedImage into GP4 file bytes
std::vector<uint8_t> encode(const IndexedImage& img);

} // namespace gp4
