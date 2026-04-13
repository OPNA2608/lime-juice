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

#include <array>
#include <cstdint>
#include <vector>

struct Rgb {
    uint8_t r, g, b;

    bool operator==(const Rgb& o) const {
        return r == o.r && g == o.g && b == o.b;
    }
};

struct IndexedImage {
    int16_t x = 0, y = 0;
    uint16_t w = 0, h = 0;
    std::array<Rgb, 16> palette = {};
    std::vector<uint8_t> pixels; // w*h palette indices (0-15), row-major
};

// GP4 palette: 16-bit BE, bits GGGG_0RRRR_0BBBB_01
Rgb gp4_palette_to_rgb(uint16_t entry);
uint16_t rgb_to_gp4_palette(Rgb color);

// GPC palette: 16-bit LE, bits GGGG_0RRRR_0BBBB_01 (same layout, different endianness)
Rgb gpc_palette_to_rgb(uint16_t entry);
uint16_t rgb_to_gpc_palette(Rgb color);
