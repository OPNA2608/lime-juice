#include "codec/gpc/gpc.h"
#include "codec/gpc/gpc_internal.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>

// All credit goes to Kirinn/Bunnylin for his great makegpc.py script which I pretty much fully copied to make this.

// build an interlace table: table[n] = source row that goes to position n
static std::vector<int> make_interlace_table(int size, int step) {
    std::vector<int> table(size);
    int write_pos = 0;

    for (int start = 0; start < step; start++) {
        for (int i = start; i < size; i += step) {
            table[write_pos++] = i;
        }
    }

    return table;
}

// split pixel indices into 4 side-by-side bitplanes per row.
// output layout per row: [plane0_bytes..., plane1_bytes..., plane2_bytes..., plane3_bytes...]
static std::vector<uint8_t> split_bitplanes(const std::vector<uint8_t>& pixels,
                                            int width, int height) {
    int plane_width = width >> 3;
    int stride = plane_width * 4;
    std::vector<uint8_t> planes(stride * height, 0);

    for (int y = 0; y < height; y++) {
        int col = 0;

        for (int x = 0; x < width; x += 8) {
            uint8_t octet[4] = {0, 0, 0, 0};

            for (int b = 0; b < 8; b++) {
                uint8_t px = pixels[y * width + x + b];

                for (int p = 0; p < 4; p++) {
                    octet[p] = (octet[p] << 1) | ((px >> p) & 1);
                }
            }

            int base = y * stride + col;

            for (int p = 0; p < 4; p++) {
                planes[base + p * plane_width] = octet[p];
            }

            col++;
        }
    }

    return planes;
}

// apply vertical interlacing: shuffle rows and XOR from bottom to top
static void apply_vertical(const std::vector<uint8_t>& src, std::vector<uint8_t>& work,
                           int stride, int height, const std::vector<int>& vrt_table) {
    // copy source rows into work buffer in interlaced order
    // work buffer has stride+1 bytes per row (1 hrz byte + stride data bytes)
    int row_size = stride + 1;

    for (int i = 0; i < height; i++) {
        int src_row = vrt_table[i];
        int work_offset = i * row_size + 1; // skip hrz byte
        std::memcpy(&work[work_offset], &src[src_row * stride], stride);
    }

    if (height <= 1) {
        return;
    }

    // XOR every row with the row above it, scanning backward from the end
    int pos = height * row_size;

    for (int i = 0; i < row_size * (height - 1); i++) {
        pos--;
        work[pos] ^= work[pos - row_size];
    }
}

// estimate compression quality for a row, lower is better.
// work_align is the row's position in the flat work buffer, used for
// flag-A/flag-B boundary alignment (not the offset into the row buffer).
static int estimate_row_bytes(const std::vector<uint8_t>& row, int row_start,
                             int stride, int work_align) {
    int compressed_bytes = 0;
    int row_idx = 0;
    int a_ctr = (work_align % 64) / 8;
    int b_ctr = work_align % 8;
    
    // process in chunks of 8:
    // OR first 8 bytes together, if all 0 then they're just one bit in the A (for loop of 8, for each loop do another 8)
    // if at any point it stops being 0, then I have a data byte to deal with and then that 8 byte cluster becomes 1 byte (B) plus one for each byte in the 8 that has data, as well as one bit.
    // repeat for next 8 bytes until we run out of stride bytes
    while(row_idx <= stride){
        // (A) byte builder loop
        for (; a_ctr < 8; a_ctr++) {
            uint8_t zero_chk = 0;

            // quick check for runs of zeros
            for (; b_ctr < 8; b_ctr++) {

                if (row_idx > stride) {
                    break;
                }

                zero_chk |= row[row_start+row_idx];

                if(zero_chk != 0x00){
                    b_ctr++;
                    break;
                }

                row_idx++;
            }

            // if zero_chk is 0, yay! add to bit ctr and carry on.
            // if zero_chk is 1, uh oh. there's data.
            if(zero_chk != 0x00){
                // one for the (B) byte we need, one for the data byte we caught.
                compressed_bytes += 2;

                // check the remaining bytes in the (B) block and add up any data found
                for(;b_ctr < 8; b_ctr++){
                    row_idx++;

                    if(row_idx > stride){
                        break;
                    }

                    if(row[row_start+row_idx] != 0x00){
                        compressed_bytes++;
                    }
                }

                // advance row_idx for next flag byte start
                row_idx++;
            }

            b_ctr = 0;
        }

        // loop has completed, add 1 extra compressed byte for the new (A) byte we're starting.
        compressed_bytes++;
        a_ctr = 0;
    }

    return compressed_bytes;
}

// apply horizontal XOR for all rows, choosing the best step per row.
// hrz_tables are pre-built interlace tables for step values 1..0x50.
static void apply_horizontal(std::vector<uint8_t>& work, int stride, int height,
                             const std::vector<std::vector<int>>& hrz_tables) {
    int row_size = stride + 1;
    std::vector<uint8_t> original(stride);
    std::vector<uint8_t> candidate(row_size);

    for (int y = 0; y < height; y++) {
        int row_start = y * row_size;

        // save original row data before we potentially overwrite it
        std::memcpy(original.data(), &work[row_start + 1], stride);

        // step=0 baseline: no XOR, data stays as-is
        work[row_start] = 0;
        int best_bytes = estimate_row_bytes(work, row_start, stride, row_start);

        // try each step value
        for (int hrz = 1; hrz <= 0x50; hrz++) {

            if (hrz > stride) {
                break;
            }

            candidate[0] = static_cast<uint8_t>(hrz);
            const auto& lookup = hrz_tables[hrz];
            uint8_t last = 0;

            for (int x = 0; x < stride; x++) {
                int src_pos = lookup[x];
                uint8_t next = original[src_pos];
                candidate[1 + src_pos] = last ^ next;
                last = next;
            }

            int bytes = estimate_row_bytes(candidate, 0, stride, row_start);

            if (bytes < best_bytes) {
                best_bytes = bytes;
                std::memcpy(&work[row_start], candidate.data(), row_size);
            }
        }
    }
}

// flag-A/flag-B compression, working backward through the data
static std::vector<uint8_t> pack_flags(const std::vector<uint8_t>& work) {
    int read_pos = static_cast<int>(work.size());
    std::vector<uint8_t> out(work.size() * 8 / 7 + 64, 0);
    int write_pos = static_cast<int>(out.size());

    uint8_t flag_a = 0;
    uint8_t flag_b = 0;

    // handle partial alignment at the end
    int flag_a_bits_left = ((read_pos + 7) >> 3) & 7;

    if (flag_a_bits_left == 0) {
        flag_a_bits_left = 8;
    }

    int flag_b_bits_left = read_pos & 7;

    if (flag_b_bits_left == 0) {
        flag_b_bits_left = 8;
    }

    while (read_pos > 0) {
        flag_b >>= 1;
        read_pos--;

        if (work[read_pos] != 0) {
            write_pos--;
            out[write_pos] = work[read_pos];
            flag_b |= 0x80;
        }

        flag_b_bits_left--;

        if (flag_b_bits_left == 0) {
            flag_b_bits_left = 8;
            flag_a >>= 1;

            if (flag_b != 0) {
                write_pos--;
                out[write_pos] = flag_b;
                flag_a |= 0x80;
            }

            flag_b = 0;
            flag_a_bits_left--;

            if (flag_a_bits_left == 0) {
                flag_a_bits_left = 8;
                write_pos--;
                out[write_pos] = flag_a;
                flag_a = 0;
            }
        }
    }

    // return only the used portion
    return std::vector<uint8_t>(out.begin() + write_pos, out.end());
}

namespace gpc {
namespace internal {

CompressedFrame encode_frame_data(const std::vector<uint8_t>& pixels,
                                  int w, int h) {
    // pad width to 8-pixel boundary
    int width = (w + 7) & ~7;

    // copy pixels, padding if needed
    std::vector<uint8_t> padded(width * h, 0);

    for (int y = 0; y < h; y++) {
        std::memcpy(&padded[y * width], &pixels[y * w], w);
    }

    // split into bitplanes
    auto planes = split_bitplanes(padded, width, h);
    int plane_width = width >> 3;
    int stride = plane_width * 4;
    int row_size = stride + 1;

    // pre-build horizontal interlace tables (shared across all vrt candidates)
    std::vector<std::vector<int>> hrz_tables(0x51);
    int hrz_max = std::min(0x50, stride);

    for (int s = 1; s <= hrz_max; s++) {
        hrz_tables[s] = make_interlace_table(stride, s);
    }

    // try vertical interlacing values, keep the smallest result
    CompressedFrame best;
    best.interleaving = 2;

    for (int vrt : {2, 1, 4}) {
        auto vrt_table = make_interlace_table(h, vrt);
        std::vector<uint8_t> work(row_size * h, 0);
        apply_vertical(planes, work, stride, h, vrt_table);
        apply_horizontal(work, stride, h, hrz_tables);
        auto compressed = pack_flags(work);

        if (best.data.empty() || compressed.size() < best.data.size()) {
            best.data = std::move(compressed);
            best.interleaving = vrt;
        }
    }

    return best;
}

} // namespace internal

std::vector<uint8_t> encode(const IndexedImage& img) {
    auto frame = internal::encode_frame_data(img.pixels, img.w, img.h);

    // build the output file
    std::vector<uint8_t> out;
    out.reserve(0x64 + frame.data.size());

    // signature (16 bytes)
    out.insert(out.end(), gpc::signature, gpc::signature + 16);

    // vertical interlacing (2 bytes LE @ 0x10)
    out.push_back(frame.interleaving & 0xFF);
    out.push_back((frame.interleaving >> 8) & 0xFF);

    // padding (2 bytes @ 0x12)
    out.push_back(0);
    out.push_back(0);

    // palette pointer (4 bytes LE @ 0x14) = 0x30
    out.push_back(0x30);
    out.push_back(0);
    out.push_back(0);
    out.push_back(0);

    // info pointer (4 bytes LE @ 0x18) = 0x54
    out.push_back(0x54);
    out.push_back(0);
    out.push_back(0);
    out.push_back(0);

    // padding to 0x30 (20 bytes)
    out.resize(0x30, 0);

    // palette block @ 0x30
    // count = 16, elem_size = 2
    out.push_back(16);
    out.push_back(0);
    out.push_back(2);
    out.push_back(0);

    // 16 palette entries (2 bytes LE each)
    for (int i = 0; i < 16; i++) {
        uint16_t entry = rgb_to_gpc_palette(img.palette[i]);
        out.push_back(entry & 0xFF);
        out.push_back((entry >> 8) & 0xFF);
    }

    // info block @ 0x54
    out.push_back(img.w & 0xFF);
    out.push_back((img.w >> 8) & 0xFF);
    out.push_back(img.h & 0xFF);
    out.push_back((img.h >> 8) & 0xFF);

    // compressed size (2 bytes LE)
    uint16_t comp_size = static_cast<uint16_t>(frame.data.size());
    out.push_back(comp_size & 0xFF);
    out.push_back((comp_size >> 8) & 0xFF);

    // padding (2 bytes)
    out.push_back(0);
    out.push_back(0);

    // bpp = 4 (2 bytes LE)
    out.push_back(4);
    out.push_back(0);

    // x, y offsets (2 bytes LE each)
    out.push_back(img.x & 0xFF);
    out.push_back((img.x >> 8) & 0xFF);
    out.push_back(img.y & 0xFF);
    out.push_back((img.y >> 8) & 0xFF);

    // padding (2 bytes)
    out.push_back(0);
    out.push_back(0);

    // compressed data @ 0x64
    out.insert(out.end(), frame.data.begin(), frame.data.end());

    return out;
}

} // namespace gpc
