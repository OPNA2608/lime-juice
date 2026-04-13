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

#include "codec/gpa/gpa.h"
#include "codec/gpc/gpc.h"
#include "codec/gpc/gpc_internal.h"

#include <cstring>
#include <stdexcept>

// helper: read a u16 LE from a byte buffer
static uint16_t read_u16(const std::vector<uint8_t>& data, size_t offset) {
    return data[offset] | (data[offset + 1] << 8);
}

// helper: read a u32 LE from a byte buffer
static uint32_t read_u32(const std::vector<uint8_t>& data, size_t offset) {
    return data[offset] | (data[offset + 1] << 8) |
           (data[offset + 2] << 16) | (data[offset + 3] << 24);
}

namespace gpa {

std::vector<IndexedImage> decode(const std::vector<uint8_t>& data) {

    // validate signature
    if (data.size() < 0x1C) {
        throw std::runtime_error("GPA file too small");
    }

    if (std::memcmp(data.data(), gpa::signature, 15) != 0) {
        throw std::runtime_error("invalid GPA signature");
    }

    // read header
    uint32_t frame_count = read_u32(data, 0x10);
    uint32_t pal_ptr = read_u32(data, 0x14);

    // read frame offset table (starts at 0x18, one u32 per frame)
    size_t offset_table_end = 0x18 + frame_count * 4;

    if (offset_table_end > data.size()) {
        throw std::runtime_error("GPA frame offset table out of bounds");
    }

    std::vector<uint32_t> frame_offsets(frame_count);

    for (uint32_t i = 0; i < frame_count; i++) {
        frame_offsets[i] = read_u32(data, 0x18 + i * 4);
    }

    // read palette (shared across all frames)
    std::array<Rgb, 16> palette = {};

    if (pal_ptr > 0 && pal_ptr + 36 <= data.size()) {

        for (int i = 0; i < 16; i++) {
            size_t off = pal_ptr + 4 + i * 2;
            uint16_t entry = read_u16(data, off);
            palette[i] = gpc_palette_to_rgb(entry);
        }
    }

    // decode each frame
    std::vector<IndexedImage> frames;
    frames.reserve(frame_count);

    for (uint32_t i = 0; i < frame_count; i++) {
        uint32_t info_ptr = frame_offsets[i];

        if (info_ptr + 10 > data.size()) {
            throw std::runtime_error("GPA frame info out of bounds");
        }

        // GPA frame info: vrt(u16), x(i16), y(i16), w(u16), h(u16)
        // x/y are stored as signed 16-bit values; preserve them exactly
        // so they can be processed through the JUICEXY GIF extension
        int interleaving = read_u16(data, info_ptr);
        int16_t x = static_cast<int16_t>(read_u16(data, info_ptr + 2));
        int16_t y = static_cast<int16_t>(read_u16(data, info_ptr + 4));
        uint16_t w = read_u16(data, info_ptr + 6);
        uint16_t h = read_u16(data, info_ptr + 8);

        // compressed data follows immediately after the 10-byte info block
        size_t data_pos = info_ptr + 10;

        IndexedImage img;
        img.x = x;
        img.y = y;
        img.w = w;
        img.h = h;
        img.palette = palette;
        img.pixels = gpc::internal::decode_frame_data(data, data_pos,
                                                      w, h, interleaving);

        frames.push_back(std::move(img));
    }

    return frames;
}

} // namespace gpa
