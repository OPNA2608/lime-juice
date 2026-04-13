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

#include <stdexcept>
#include <vector>

// helper: append a u16 LE to a byte vector
static void emit_u16(std::vector<uint8_t>& out, uint16_t val) {
    out.push_back(val & 0xFF);
    out.push_back((val >> 8) & 0xFF);
}

// helper: append a u32 LE to a byte vector
static void emit_u32(std::vector<uint8_t>& out, uint32_t val) {
    out.push_back(val & 0xFF);
    out.push_back((val >> 8) & 0xFF);
    out.push_back((val >> 16) & 0xFF);
    out.push_back((val >> 24) & 0xFF);
}

namespace gpa {

std::vector<uint8_t> encode(const std::vector<IndexedImage>& frames) {

    if (frames.empty()) {
        throw std::runtime_error("no frames to encode");
    }

    uint32_t frame_count = static_cast<uint32_t>(frames.size());

    // compress each frame's pixel data independently
    struct FrameData {
        gpc::internal::CompressedFrame compressed;
        uint16_t x, y, w, h;
    };

    std::vector<FrameData> frame_data;
    frame_data.reserve(frame_count);

    for (const auto& frame : frames) {
        FrameData fd;
        fd.compressed = gpc::internal::encode_frame_data(frame.pixels,
                                                         frame.w, frame.h);
        fd.x = frame.x;
        fd.y = frame.y;
        fd.w = frame.w;
        fd.h = frame.h;
        frame_data.push_back(std::move(fd));
    }

    // assemble the GPA file from the compressed frames
    std::vector<uint8_t> encoded_gpa_bytes;
    encoded_gpa_bytes.reserve(0x64 + frame_count * 64);

    // write signature before any data (16 bytes)
    encoded_gpa_bytes.insert(encoded_gpa_bytes.end(), gpa::signature, gpa::signature + 16);

    // write frame count as u32 (4 bytes)
    emit_u32(encoded_gpa_bytes, frame_count);

    // write palette pointer (has to skip frame offsets block, so calculated as 0x18 + frame count * 4 bytes per offset) (4 bytes)
    emit_u32(encoded_gpa_bytes, 0x18+(frame_count*4));

    // write frame offset list (4 bytes per frame)
    int offset = 0x18+(frame_count*4)+36;

    for (int i = 0;i < frame_count;i++){
        // size per frame is calculated as (10-byte header) + (compressed data length for frame)
        emit_u32(encoded_gpa_bytes, offset);

        // calculate next absolute offset
        offset += 10 + frame_data[i].compressed.data.size();
    }

    // write palette block, 4 byte header and 32 bytes of colour data (36 bytes)
    // this will almost always be 16 colours and 2 bytes per colour, but i'm calculating it just in case
    emit_u16(encoded_gpa_bytes, frames[0].palette.size());
    emit_u16(encoded_gpa_bytes, sizeof(uint16_t));

    // you know, it's a bit odd that the entire gpa only has one palette, even though it's a collection of individual packed GPC images
    // i suppose it was probably to make it easier to render on the PC98

    // write colours converted from RGB to GPC colourspace
    for(int i=0; i < frames[0].palette.size(); i++){
        emit_u16(encoded_gpa_bytes, rgb_to_gpc_palette(frames[0].palette[i]));
    }

    // finally, write actual frames (variable bytes per frame)
    for (int i = 0;i < frame_count;i++){
        // start with header: [vrt_il u16] [x u16] [y u16] [w u16] [h u16]
        emit_u16(encoded_gpa_bytes, frame_data[i].compressed.interleaving);
        emit_u16(encoded_gpa_bytes, frame_data[i].x);
        emit_u16(encoded_gpa_bytes, frame_data[i].y);
        emit_u16(encoded_gpa_bytes, frame_data[i].w);
        emit_u16(encoded_gpa_bytes, frame_data[i].h);

        // then copy actual data into the vector all at once
        encoded_gpa_bytes.insert(encoded_gpa_bytes.end(), frame_data[i].compressed.data.begin(), frame_data[i].compressed.data.end());
    }

    return encoded_gpa_bytes;
}

} // namespace gpa
