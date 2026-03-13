#include "codec/gp4/gp4.h"
#include "codec/gp4/gp4_color_table.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace gp4 {

// ============================================================
// bit reader (MSB-first)
// ============================================================

class BitReader {
    const uint8_t* data_;
    size_t size_;
    size_t byte_pos_ = 0;
    int bit_pos_ = 0; // 0 = MSB (bit 7), 7 = LSB (bit 0)

public:
    BitReader(const uint8_t* data, size_t size)
        : data_(data), size_(size) {}

    bool at_end() const {
        return byte_pos_ >= size_;
    }

    int read_bit() {

        if (byte_pos_ >= size_) {
            return 0;
        }

        int bit = (data_[byte_pos_] >> (7 - bit_pos_)) & 1;
        bit_pos_++;

        if (bit_pos_ >= 8) {
            bit_pos_ = 0;
            byte_pos_++;
        }

        return bit;
    }

    // read n bits MSB-first, return as unsigned integer
    int read_bits(int n) {
        int val = 0;

        for (int i = 0; i < n; i++) {
            val = (val << 1) | read_bit();
        }

        return val;
    }

    // count consecutive 1-bits, consume trailing 0-bit
    int read_elias_gamma() {
        int count = 0;

        while (read_bit() == 1) {
            count++;
        }

        return count;
    }
};

// ============================================================
// copy offset form 2 lookup table
// ============================================================

static const int x0y_dy_table[8] = {-16, -8, -6, -5, -4, -3, -2, -1};

// ============================================================
// decoder
// ============================================================

static constexpr int CANVAS_H = 400;

IndexedImage decode(const std::vector<uint8_t>& data, int canvas_w) {

    if (data.size() < 40) {
        throw std::runtime_error("GP4 file too small");
    }

    // parse header (big-endian)
    uint16_t x0 = (data[0] << 8) | data[1];
    uint16_t y0 = (data[2] << 8) | data[3];
    uint16_t w  = ((data[4] << 8) | data[5]) + 1;
    uint16_t h  = ((data[6] << 8) | data[7]) + 1;

    uint16_t x1 = x0 + w - 1;
    uint16_t y1 = y0 + h - 1;

    if (x1 >= canvas_w || y1 >= CANVAS_H) {
        throw std::runtime_error("GP4 dimensions exceed canvas");
    }

    // parse palette
    IndexedImage img;
    img.x = x0;
    img.y = y0;
    img.w = w;
    img.h = h;

    for (int i = 0; i < 16; i++) {
        uint16_t entry = (data[8 + i * 2] << 8) | data[8 + i * 2 + 1];
        img.palette[i] = gp4_palette_to_rgb(entry);
    }

    // initialize canvas buffer
    std::vector<uint8_t> canvas(canvas_w * CANVAS_H, 0);

    // initialize color table and bit reader
    ColorTable table;
    BitReader bits(data.data() + 40, data.size() - 40);

    // cursor position (column-major traversal)
    int cx = x0;
    int cy = y0;

    // advance cursor by one row, handle column wrapping
    auto advance = [&]() -> bool {
        cy++;

        if (cy > y1) {
            cx += 4;
            cy = y0;
            return true; // crossed column boundary
        }

        return false;
    };

    // main decode loop
    while (cx <= x1 && !bits.at_end()) {

        int cmd = bits.read_bit();

        if (cmd == 0) {

            // draw command: 4 elias-gamma color indices
            for (int i = 0; i < 4; i++) {
                int pos = bits.read_elias_gamma();
                uint8_t color = table.poke(pos);
                int px = cx + i;

                if (px < canvas_w && cy < CANVAS_H) {
                    canvas[cy * canvas_w + px] = color;
                }
            }

            if (advance()) {
                table.reset();
            }

        } else {

            // copy command: offset + height
            int dx, dy;
            int next_bit = bits.read_bit();

            if (next_bit == 0) {

                // form x1y: dx = -1
                dx = -1;
                dy = bits.read_bits(4) - 8;
            } else {
                int next_bit2 = bits.read_bit();

                if (next_bit2 == 0) {

                    // form x0y: dx = 0, dy from lookup
                    dx = 0;
                    int idx = bits.read_bits(3);
                    dy = x0y_dy_table[idx];
                } else {

                    // form x2y: dx from elias-gamma, dy from 4 bits
                    int eg = bits.read_elias_gamma();
                    dx = -(eg + 2);
                    dy = bits.read_bits(4) - 8;
                }
            }

            // decode height
            int height;
            int h_bit = bits.read_bit();

            if (h_bit == 0) {

                // h1: [2..3]
                height = bits.read_bits(1) + 2;
            } else {
                int h_bit2 = bits.read_bit();

                if (h_bit2 == 0) {

                    // h2: [4..7]
                    height = bits.read_bits(2) + 4;
                } else {
                    int h_bit3 = bits.read_bit();

                    if (h_bit3 == 0) {

                        // h3: [8..15]
                        height = bits.read_bits(3) + 8;
                    } else {

                        // h4 or h5: read 6-bit payload after "111" prefix
                        int payload = bits.read_bits(6);

                        if (payload == 63) {

                            // h5: all 6 bits are ones, then 10-bit value [79..1102]
                            height = bits.read_bits(10) + 79;
                        } else {

                            // h4: 6-bit value [16..78]
                            height = payload + 16;
                        }
                    }
                }
            }

            // execute copy
            for (int row = 0; row < height && cx <= x1; row++) {
                int sx = cx + 4 * dx;

                for (int i = 0; i < 4; i++) {
                    int src_x = sx + i;
                    int src_y = cy + dy;

                    if (src_x >= 0 && src_x < canvas_w &&
                        src_y >= 0 && src_y < CANVAS_H) {
                        int dst_x = cx + i;

                        if (dst_x < canvas_w && cy < CANVAS_H) {
                            canvas[cy * canvas_w + dst_x] =
                                canvas[src_y * canvas_w + src_x];
                        }
                    }
                }

                if (advance()) {
                    table.reset();
                }
            }
        }
    }

    // extract bounding rectangle from canvas (row-major)
    img.pixels.resize(static_cast<size_t>(w) * h);

    for (int row = 0; row < h; row++) {

        for (int col = 0; col < w; col++) {
            img.pixels[row * w + col] = canvas[(y0 + row) * canvas_w + (x0 + col)];
        }
    }

    // if a custom canvas width was specified, rewrap the pixel data.
    // this reinterprets the linear pixel stream at the new width.
    if (canvas_w != 640) {
        size_t total = img.pixels.size();
        uint16_t new_w = static_cast<uint16_t>(canvas_w);
        uint16_t new_h = static_cast<uint16_t>((total + new_w - 1) / new_w);

        std::vector<uint8_t> rewrapped(static_cast<size_t>(new_w) * new_h, 0);

        for (size_t i = 0; i < total; i++) {
            rewrapped[i] = img.pixels[i];
        }

        img.pixels = std::move(rewrapped);
        img.x = 0;
        img.y = 0;
        img.w = new_w;
        img.h = new_h;
    }

    return img;
}

} // namespace gp4
