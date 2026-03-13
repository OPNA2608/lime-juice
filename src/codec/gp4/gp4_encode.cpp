#include "codec/gp4/gp4.h"
#include "codec/gp4/gp4_color_table.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace gp4 {

// ============================================================
// bit writer (MSB-first)
// ============================================================

class BitWriter {
    std::vector<uint8_t> bytes_;
    uint8_t current_ = 0;
    int bit_pos_ = 0;

public:
    void write_bit(int b) {
        current_ = (current_ << 1) | (b & 1);
        bit_pos_++;

        if (bit_pos_ >= 8) {
            bytes_.push_back(current_);
            current_ = 0;
            bit_pos_ = 0;
        }
    }

    // write n bits from val, MSB-first
    void write_bits(int val, int n) {
        for (int i = n - 1; i >= 0; i--) {
            write_bit((val >> i) & 1);
        }
    }

    // write val 1-bits followed by a 0-bit
    void write_elias_gamma(int val) {
        write_bits(-1, val);
        write_bit(0);
    }

    // pad remaining bits with zeros and return the byte buffer
    std::vector<uint8_t> finish() {

        if (bit_pos_ > 0) {
            current_ <<= (8 - bit_pos_);
            bytes_.push_back(current_);
        }

        return bytes_;
    }
};

// ============================================================
// copy offset lookup table (matches decoder's x0y_dy_table)
// ============================================================

static const int x0y_dy_table[8] = {-16, -8, -6, -5, -4, -3, -2, -1};

// ============================================================
// height encoding
// ============================================================

static void encode_height(BitWriter& bits, int height) {

    if (height <= 3) {
        // h1: prefix 0, 1-bit payload [2..3]
        bits.write_bit(0);
        bits.write_bits(height - 2, 1);
    } else if (height <= 7) {
        // h2: prefix 10, 2-bit payload [4..7]
        bits.write_bit(1);
        bits.write_bit(0);
        bits.write_bits(height - 4, 2);
    } else if (height <= 15) {
        // h3: prefix 110, 3-bit payload [8..15]
        bits.write_bit(1);
        bits.write_bit(1);
        bits.write_bit(0);
        bits.write_bits(height - 8, 3);
    } else if (height <= 78) {
        // h4: prefix 111, 6-bit payload [16..78]
        bits.write_bit(1);
        bits.write_bit(1);
        bits.write_bit(1);
        bits.write_bits(height - 16, 6);
    } else {
        // h5: prefix 111, 6-bit escape (all ones), 10-bit payload [79..1102]
        bits.write_bit(1);
        bits.write_bit(1);
        bits.write_bit(1);
        bits.write_bits(63, 6);
        bits.write_bits(height - 79, 10);
    }
}

// ============================================================
// copy command bit cost
// ============================================================

static int offset_bit_cost(int dx) {

    if (dx == -1 || dx == 0) {
        return 5;
    }

    // x2y: 2 prefix bits + elias gamma of -(dx+2) + 4-bit dy
    int eg = -(dx + 2);
    return 2 + (eg + 1) + 4;
}

static int height_bit_cost(int height) {

    if (height <= 3) {
        return 2;
    } else if (height <= 7) {
        return 4;
    } else if (height <= 15) {
        return 6;
    } else if (height <= 78) {
        return 9;
    }

    return 19;
}

static int copy_bit_cost(int dx, int height) {
    // 1 cmd bit + offset + height
    return 1 + offset_bit_cost(dx) + height_bit_cost(height);
}

// ============================================================
// encoder
// ============================================================

static constexpr int CANVAS_W = 640;
static constexpr int CANVAS_H = 400;
static constexpr int MAX_COPY_HEIGHT = 1102;

// estimated average bit cost of a draw command per row.
// draw = 1 cmd bit + 4 elias gamma values; typical MRU positions
// average around 1-2 each, so ~9 bits per row is a reasonable estimate.
static constexpr int DRAW_COST_ESTIMATE = 9;

std::vector<uint8_t> encode(const IndexedImage& img) {

    if (img.w == 0 || img.h == 0) {
        throw std::runtime_error("GP4 image has zero dimensions");
    }

    if (img.w % 4 != 0) {
        throw std::runtime_error("GP4 image width must be a multiple of 4 (got " +
            std::to_string(img.w) + ")");
    }

    if (img.x + img.w > CANVAS_W || img.y + img.h > CANVAS_H) {
        throw std::runtime_error("GP4 image exceeds 640x400 canvas");
    }

    // place image pixels onto 640x400 canvas
    std::vector<uint8_t> canvas(CANVAS_W * CANVAS_H, 0);

    for (int row = 0; row < img.h; row++) {

        for (int col = 0; col < img.w; col++) {
            canvas[(img.y + row) * CANVAS_W + (img.x + col)] =
                img.pixels[row * img.w + col];
        }
    }

    int x0 = img.x, y0 = img.y;
    int x1 = img.x + img.w - 1, y1 = img.y + img.h - 1;
    int cx = x0, cy = y0;

    // advance cursor by one row, wrap columns
    auto advance = [&]() -> bool {
        cy++;

        if (cy > y1) {
            cx += 4;
            cy = y0;
            return true;
        }

        return false;
    };

    auto pixel = [&](int x, int y) -> uint8_t {
        return canvas[y * CANVAS_W + x];
    };

    // count how many consecutive 4-pixel rows match at offset (dx, dy)
    auto match_length = [&](int dx, int dy) -> int {
        int len = 0;
        int tcx = cx, tcy = cy;

        while (tcx <= x1 && len < MAX_COPY_HEIGHT) {
            int sx = tcx + 4 * dx;
            int sy = tcy + dy;

            if (sx < 0 || sx + 3 >= CANVAS_W || sy < 0 || sy >= CANVAS_H) {
                break;
            }

            bool match = true;

            for (int i = 0; i < 4; i++) {

                if (pixel(tcx + i, tcy) != pixel(sx + i, sy)) {
                    match = false;
                    break;
                }
            }

            if (!match) {
                break;
            }

            len++;
            tcy++;

            if (tcy > y1) {
                tcx += 4;
                tcy = y0;
            }
        }

        return len;
    };

    ColorTable table;
    BitWriter bits;

    while (cx <= x1) {

        // search for best copy match by bit savings
        int best_savings = 0;
        int best_len = 0;
        int best_dx = 0, best_dy = 0;

        // evaluate a candidate: keep it if it saves more bits than the current best
        auto try_candidate = [&](int dx, int dy) {
            int len = match_length(dx, dy);

            if (len < 2) {
                return;
            }

            int savings = len * DRAW_COST_ESTIMATE - copy_bit_cost(dx, len);

            if (savings > best_savings) {
                best_savings = savings;
                best_len = len;
                best_dx = dx;
                best_dy = dy;
            }
        };

        // x0y candidates: dx=0, dy from lookup table
        for (int idx = 0; idx < 8; idx++) {
            int dy = x0y_dy_table[idx];

            if (cy + dy < 0 || cy + dy >= CANVAS_H) {
                continue;
            }

            try_candidate(0, dy);
        }

        // x1y and x2y candidates: dx in [-16..-1], dy in [-8..7]
        for (int dx = -16; dx <= -1; dx++) {

            if (cx + 4 * dx < 0) {
                continue;
            }

            for (int dy = -8; dy <= 7; dy++) {

                if (cy + dy < 0 || cy + dy >= CANVAS_H) {
                    continue;
                }

                try_candidate(dx, dy);
            }
        }

        if (best_savings > 0) {

            // emit copy command
            bits.write_bit(1);

            if (best_dx == -1) {
                // x1y form: prefix 0, 4-bit dy
                bits.write_bit(0);
                bits.write_bits(best_dy + 8, 4);
            } else if (best_dx == 0) {
                // x0y form: prefix 10, 3-bit lookup index
                bits.write_bit(1);
                bits.write_bit(0);
                int idx = 0;

                for (int i = 0; i < 8; i++) {

                    if (x0y_dy_table[i] == best_dy) {
                        idx = i;
                        break;
                    }
                }

                bits.write_bits(idx, 3);
            } else {
                // x2y form: prefix 11, elias-gamma dx, 4-bit dy
                bits.write_bit(1);
                bits.write_bit(1);
                bits.write_elias_gamma(-(best_dx + 2));
                bits.write_bits(best_dy + 8, 4);
            }

            encode_height(bits, best_len);

            // advance cursor by copy height
            for (int r = 0; r < best_len; r++) {

                if (advance()) {
                    table.reset();
                }
            }

        } else {

            // emit draw command: 4 elias-gamma color indices
            bits.write_bit(0);

            for (int i = 0; i < 4; i++) {
                uint8_t color = pixel(cx + i, cy);
                int pos = table.peek(color);
                table.poke(pos);
                bits.write_elias_gamma(pos);
            }

            if (advance()) {
                table.reset();
            }
        }
    }

    // end marker: 7 zero bits
    for (int i = 0; i < 7; i++) {
        bits.write_bit(0);
    }

    auto bitstream = bits.finish();

    // assemble output: 8-byte header + 32-byte palette + bitstream
    std::vector<uint8_t> out(40 + bitstream.size());

    // header (big-endian)
    out[0] = (img.x >> 8) & 0xFF;
    out[1] = img.x & 0xFF;
    out[2] = (img.y >> 8) & 0xFF;
    out[3] = img.y & 0xFF;
    out[4] = ((img.w - 1) >> 8) & 0xFF;
    out[5] = (img.w - 1) & 0xFF;
    out[6] = ((img.h - 1) >> 8) & 0xFF;
    out[7] = (img.h - 1) & 0xFF;

    // palette (16 entries, big-endian)
    for (int i = 0; i < 16; i++) {
        uint16_t entry = rgb_to_gp4_palette(img.palette[i]);
        out[8 + i * 2] = (entry >> 8) & 0xFF;
        out[8 + i * 2 + 1] = entry & 0xFF;
    }

    // bitstream
    std::memcpy(out.data() + 40, bitstream.data(), bitstream.size());

    return out;
}

} // namespace gp4
