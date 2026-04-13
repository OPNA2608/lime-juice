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

namespace gpc {

// standard GPC file signature
inline constexpr const char* signature = "PC98)GPCFILE   ";

// decode a GPC file from raw bytes into an IndexedImage
IndexedImage decode(const std::vector<uint8_t>& data);

// encode an IndexedImage into GPC file bytes
std::vector<uint8_t> encode(const IndexedImage& img);

} // namespace gpc
