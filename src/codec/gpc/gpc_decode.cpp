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

#include "codec/gpc/gpc.h"
#include "codec/gpc/gpc_internal.h"

#include <cstring>
#include <stdexcept>

// All credit goes to "morkt" for their work creating & open-sourcing GARBro, which I basically copied to make this.

// flag-A/flag-B decompression.
// each flag-A bit controls a group of 8 bytes:
//   flag-A bit = 0: output 8 zero bytes
//   flag-A bit = 1: read a flag-B byte, where each flag-B bit controls one byte:
//     flag-B bit = 0: output a zero byte
//     flag-B bit = 1: read and output a literal byte
// flag-A bits are read MSB-first from each control byte.
// when all 8 flag-A bits are consumed, the next control byte is read.
static void unpack_data(const std::vector<uint8_t>& input, size_t& pos,
                        std::vector<uint8_t>& output) {
    int idx = 0;

    while (idx < output.size()){
        uint8_t bitA = input[pos];
        pos++;

        // process first (flagA) byte of input one bit at a time, msb first (& 0x80, rot << 1)
        for( int i = 7; i >= 0 ; i-- ){
            if(idx >= output.size()){break;}

            if (bitA & 0x80){
                // if msb of input is 1, process the next (flagB) byte one bit at a time.
                uint8_t bitB = input[pos];
                pos++;

                for( int j = 7; j >= 0 ; j-- ){
                    if(idx >= output.size()){break;}

                    // if msb of inputB is 1, write next input byte directly into output and move on a bit in input
                    // if msb of inputB is 0, just skip a byte ahead in output and move on a bit in input
                    if (bitB & 0x80){
                        output[idx] = input[pos];
                        pos++;
                    }

                    // move ahead one bit in inputB and index
                    bitB <<= 1;
                    idx++;
                }
            }else{
                // if msb of input uint is 0, skip 8 bytes ahead in output and move on a bit in input
                idx += 8;
            }

            // move ahead one bit in input
            bitA <<= 1;
        }
    }
}

// undo horizontal XOR interlacing for one row.
// the first byte of each row is the interlace step value.
// bytes after index 0 are XOR'd with the byte that is `step` positions
// behind, cycling through columns at the step interval.
static void restore_horizontal(std::vector<uint8_t>& data, size_t row_start,
                               int stride) {
    int step = data[row_start];

    if (step == 0) {
        return;
    }

    uint8_t last = 0;

    for (int i = 0; i < step; i++) {
        int pos = 1 + i;

        while (pos < stride) {
            data[row_start + pos] ^= last;
            last = data[row_start + pos];
            pos += step;
        }
    }
}

// merge 4 bitplanes into pixel indices (0-15), handling row interlacing.
// input rows are sequential in data; output rows are distributed by the
// interlacing step to reconstruct the original image order.
static std::vector<uint8_t> merge_bitplanes(const std::vector<uint8_t>& data,
                                            int plane_stride, int width,
                                            int height, int interleaving) {
    int pixel_stride = plane_stride * 4; // bytes of pixel data per row (nibble-packed)
    std::vector<uint8_t> nibble_buf(pixel_stride * height, 0);

    int interleaving_step = pixel_stride * interleaving;
    int src_row = 1; // skip interlace byte of first row
    int dst_row = 0;
    int wrap_idx = 0;
    int row_size = plane_stride * 4 + 1;

    for (int y = 0; y < height; y++) {

        if (dst_row >= static_cast<int>(nibble_buf.size())) {
            dst_row = pixel_stride * (++wrap_idx);
        }

        int p0 = src_row;
        int p1 = p0 + plane_stride;
        int p2 = p1 + plane_stride;
        int p3 = p2 + plane_stride;
        src_row = p3 + plane_stride + 1; // skip to next row's interlace byte

        int dst = dst_row;

        for (int x = 0; x < plane_stride; x++) {
            uint8_t b0 = data[p0++];
            uint8_t b1 = data[p1++];
            uint8_t b2 = data[p2++];
            uint8_t b3 = data[p3++];

            // extract 8 pixels from 4 bitplane bytes, packed as nibble pairs
            for (int j = 0; j < 8; j += 2) {
                uint8_t px = static_cast<uint8_t>(
                    (((b0 << j) & 0x80) >> 3) |
                    (((b1 << j) & 0x80) >> 2) |
                    (((b2 << j) & 0x80) >> 1) |
                    (((b3 << j) & 0x80)     ));
                px |= static_cast<uint8_t>(
                    (((b0 << j) & 0x40) >> 6) |
                    (((b1 << j) & 0x40) >> 5) |
                    (((b2 << j) & 0x40) >> 4) |
                    (((b3 << j) & 0x40) >> 3));
                nibble_buf[dst++] = px;
            }
        }

        dst_row += interleaving_step;
    }

    // unpack nibble pairs into individual pixel indices
    std::vector<uint8_t> pixels(width * height);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t byte = nibble_buf[y * pixel_stride + x / 2];

            if (x % 2 == 0) {
                pixels[y * width + x] = (byte >> 4) & 0x0F;
            } else {
                pixels[y * width + x] = byte & 0x0F;
            }
        }
    }

    return pixels;
}

namespace gpc {
namespace internal {

std::vector<uint8_t> decode_frame_data(const std::vector<uint8_t>& data,
                                       size_t& pos, int w, int h,
                                       int interleaving) {
    int plane_stride = (w + 7) >> 3;
    int row_size = plane_stride * 4 + 1; // 4 planes + interlace byte
    std::vector<uint8_t> raw(row_size * h, 0);

    unpack_data(data, pos, raw);

    // restore XOR transforms.
    // horizontal and vertical must be applied row-by-row in a single pass,
    // because each row's vertical XOR depends on the fully-restored previous row.
    for (int y = 0; y < h; y++) {
        restore_horizontal(raw, y * row_size, row_size);

        if (y > 0) {
            size_t row = static_cast<size_t>(y) * row_size;
            size_t prev = row - row_size;
            int length = (row_size - 1) & -4;

            for (int x = 1; x <= length; x++) {
                raw[row + x] ^= raw[prev + x];
            }
        }
    }

    // merge bitplanes into pixel indices
    return merge_bitplanes(raw, plane_stride, w, h, interleaving);
}

} // namespace internal

IndexedImage decode(const std::vector<uint8_t>& data) {

    // validate signature (need at least 0x1C bytes for base header + pointers)
    if (data.size() < 0x1C) {
        throw std::runtime_error("GPC file too small");
    }

    if (std::memcmp(data.data(), gpc::signature, 15) != 0) {
        throw std::runtime_error("invalid GPC signature");
    }

    // read header
    int interleaving = data[0x10] | (data[0x11] << 8);
    uint32_t pal_ptr = data[0x14] | (data[0x15] << 8) |
                       (data[0x16] << 16) | (data[0x17] << 24);
    uint32_t info_ptr = data[0x18] | (data[0x19] << 8) |
                        (data[0x1A] << 16) | (data[0x1B] << 24);

    // read palette
    IndexedImage img;

    if (pal_ptr > 0 && pal_ptr + 36 <= data.size()) {

        for (int i = 0; i < 16; i++) {
            size_t off = pal_ptr + 4 + i * 2;
            uint16_t entry = data[off] | (data[off + 1] << 8);
            img.palette[i] = gpc_palette_to_rgb(entry);
        }
    }

    // read image info
    if (info_ptr + 0x10 > data.size()) {
        throw std::runtime_error("GPC info block out of bounds");
    }

    img.w = data[info_ptr] | (data[info_ptr + 1] << 8);
    img.h = data[info_ptr + 2] | (data[info_ptr + 3] << 8);
    img.x = data[info_ptr + 0x0A] | (data[info_ptr + 0x0B] << 8);
    img.y = data[info_ptr + 0x0C] | (data[info_ptr + 0x0D] << 8);

    // decompress frame data
    size_t data_pos = info_ptr + 0x10;
    img.pixels = internal::decode_frame_data(data, data_pos, img.w, img.h,
                                             interleaving);

    return img;
}

} // namespace gpc
