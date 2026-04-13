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

#include "image/image.h"

// GP4 palette entry: 16-bit BE, bits GGGG_xRRRR_xBBBB_01
// g = bits 15-12, r = bits 10-7, b = bits 5-2
Rgb gp4_palette_to_rgb(uint16_t entry) {
    uint8_t g = ((entry >> 12) & 0x0F) * 0x11;
    uint8_t r = ((entry >> 7) & 0x0F) * 0x11;
    uint8_t b = ((entry >> 2) & 0x0F) * 0x11;
    return {r, g, b};
}

uint16_t rgb_to_gp4_palette(Rgb color) {
    uint16_t g = (color.g / 0x11) & 0x0F;
    uint16_t r = (color.r / 0x11) & 0x0F;
    uint16_t b = (color.b / 0x11) & 0x0F;
    return (g << 12) | (r << 7) | (b << 2) | 0x01;
}

// GPC palette entry: 2 bytes LE, byte0 = RRRR_BBBB, byte1 = 0000_GGGG
// as uint16 LE: r = bits 7-4, b = bits 3-0, g = bits 11-8
Rgb gpc_palette_to_rgb(uint16_t entry) {
    uint8_t r = ((entry >> 4) & 0x0F) * 0x11;
    uint8_t b = (entry & 0x0F) * 0x11;
    uint8_t g = ((entry >> 8) & 0x0F) * 0x11;
    return {r, g, b};
}

uint16_t rgb_to_gpc_palette(Rgb color) {
    uint16_t r = (color.r / 0x11) & 0x0F;
    uint16_t b = (color.b / 0x11) & 0x0F;
    uint16_t g = (color.g / 0x11) & 0x0F;
    return (r << 4) | b | (g << 8);
}
