#include "image/gif_io.h"

#include <lecram/gifdec.h>
#include <lecram/gifenc.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <utility>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

// ============================================================
// offset metadata frame
//
// per-frame x/y can be stored negative; the sign appears to be
// a flag, recording offset from some unknown position in the
// game window. IF YOU KNOW WHAT THESE NEGATIVE VALUES MEAN,
// PLEASE TELL ME!! for now i'm just making a custom metadata
// frame to store the raw offsets so I can restore them properly
// later. when a GPA file has negative frame offsets, the GIF 
// cannot natively represent them (GIF offsets are unsigned).
//
// the frame encodes offsets as nibble-packed pixel data:
//   magic "JX" (4 pixels) + frame count (2 pixels) +
//   per-frame x,y as int16 LE (8 pixels each)
//
// each byte is stored as two pixels: high nibble first, then
// low nibble. all pixel values are 0-15 (valid palette indices).
// ============================================================

// encode a byte as two nibble pixels (high nibble first)
static void emit_byte(std::vector<uint8_t>& pixels, uint8_t b) {
    pixels.push_back((b >> 4) & 0x0F);
    pixels.push_back(b & 0x0F);
}

// decode two nibble pixels back into a byte
static uint8_t read_packed_byte(const std::vector<uint8_t>& pixels, size_t byte_idx) {
    return static_cast<uint8_t>((pixels[byte_idx * 2] << 4) | pixels[byte_idx * 2 + 1]);
}

// build the pixel data for a metadata frame
static std::vector<uint8_t>
encode_offset_pixels(const std::vector<IndexedImage>& frames) {
    std::vector<uint8_t> pixels;

    // magic "JX"
    emit_byte(pixels, 0x4A);
    emit_byte(pixels, 0x58);

    // frame count
    emit_byte(pixels, static_cast<uint8_t>(frames.size()));

    // per-frame signed offsets as int16 LE
    for (const auto& f : frames) {
        emit_byte(pixels, static_cast<uint8_t>(f.x & 0xFF));
        emit_byte(pixels, static_cast<uint8_t>((f.x >> 8) & 0xFF));
        emit_byte(pixels, static_cast<uint8_t>(f.y & 0xFF));
        emit_byte(pixels, static_cast<uint8_t>((f.y >> 8) & 0xFF));
    }

    return pixels;
}

// try to extract signed offsets from a metadata frame's pixels.
// returns empty vector if the frame is not a valid metadata frame.
static std::vector<std::pair<int16_t, int16_t>>
decode_offset_pixels(const std::vector<uint8_t>& pixels) {
    std::vector<std::pair<int16_t, int16_t>> offsets;

    // need at least 3 bytes = 6 pixels for magic + count
    if (pixels.size() < 6) {
        return offsets;
    }

    // check all pixels are valid nibbles (0-15)
    for (size_t i = 0; i < 6; i++) {

        if (pixels[i] > 0x0F) {
            return offsets;
        }
    }

    // check magic "JX"
    if (read_packed_byte(pixels, 0) != 0x4A ||
        read_packed_byte(pixels, 1) != 0x58) {
        return offsets;
    }

    uint8_t frame_count = read_packed_byte(pixels, 2);

    // verify we have enough pixel data
    size_t needed_pixels = static_cast<size_t>(3 + frame_count * 4) * 2;

    if (pixels.size() < needed_pixels) {
        return offsets;
    }

    for (int i = 0; i < frame_count; i++) {
        size_t base = 3 + i * 4;
        uint8_t x_lo = read_packed_byte(pixels, base);
        uint8_t x_hi = read_packed_byte(pixels, base + 1);
        uint8_t y_lo = read_packed_byte(pixels, base + 2);
        uint8_t y_hi = read_packed_byte(pixels, base + 3);
        int16_t x = static_cast<int16_t>(x_lo | (x_hi << 8));
        int16_t y = static_cast<int16_t>(y_lo | (y_hi << 8));
        offsets.emplace_back(x, y);
    }

    return offsets;
}

// check if any frame has a negative offset
static bool has_negative_offsets(const std::vector<IndexedImage>& frames) {

    for (const auto& f : frames) {

        if (f.x < 0 || f.y < 0) {
            return true;
        }
    }

    return false;
}

// ============================================================
// GIF reading (via gifdec)
// ============================================================

std::vector<IndexedImage> load_gif(const std::string& path) {
    gd_GIF* gif = gd_open_gif(path.c_str());

    if (!gif) {
        throw std::runtime_error("cannot open GIF: " + path);
    }

    // read global palette (first 16 colors)
    std::array<Rgb, 16> palette = {};
    int num_colors = std::min(gif->gct.size, 16);

    for (int i = 0; i < num_colors; i++) {
        palette[i] = {
            gif->gct.colors[i * 3],
            gif->gct.colors[i * 3 + 1],
            gif->gct.colors[i * 3 + 2]
        };
    }

    std::vector<IndexedImage> frames;

    while (gd_get_frame(gif) == 1) {
        IndexedImage img;
        img.x = gif->fx;
        img.y = gif->fy;
        img.w = gif->fw;
        img.h = gif->fh;
        img.palette = palette;

        // extract indexed pixel data from the frame rectangle
        size_t pixel_count = static_cast<size_t>(img.w) * img.h;
        img.pixels.resize(pixel_count);

        for (int row = 0; row < img.h; row++) {

            for (int col = 0; col < img.w; col++) {
                img.pixels[row * img.w + col] =
                    gif->frame[(gif->fy + row) * gif->width + gif->fx + col];
            }
        }

        frames.push_back(std::move(img));
    }

    // check for metadata in the canvas bottom row.
    // the metadata row survives editor re-saves because it is
    // part of the rendered visual state of the final frame.
    if (gif->height >= 2 && frames.size() >= 2) {
        int bottom_y = gif->height - 1;
        std::vector<uint8_t> bottom_row(gif->width);

        for (int col = 0; col < gif->width; col++) {
            bottom_row[col] = gif->frame[bottom_y * gif->width + col];
        }

        auto offsets = decode_offset_pixels(bottom_row);

        if (!offsets.empty() &&
            offsets.size() == frames.size() - 1) {

            // remove the metadata frame (last one)
            frames.pop_back();

            // crop all frames that extend into the metadata row
            for (auto& f : frames) {

                if (f.y + f.h > bottom_y) {
                    uint16_t new_h = static_cast<uint16_t>(bottom_y - f.y);
                    f.pixels.resize(static_cast<size_t>(f.w) * new_h);
                    f.h = new_h;
                }
            }

            // restore original signed offsets
            for (size_t i = 0; i < offsets.size() && i < frames.size(); i++) {
                frames[i].x = offsets[i].first;
                frames[i].y = offsets[i].second;
            }
        }
    }

    gd_close_gif(gif);

    if (frames.empty()) {
        throw std::runtime_error("no frames found in GIF: " + path);
    }

    return frames;
}

// ============================================================
// GIF writing (via gifenc)
// ============================================================

bool save_gif(const std::string& path,
              const std::vector<IndexedImage>& frames) {

    if (frames.empty()) {
        throw std::runtime_error("no frames to write");
    }

    bool needs_meta = has_negative_offsets(frames);

    // compute visual offsets: if any frame has negative coords,
    // shift everything so the minimum is at (0, 0)
    int16_t min_x = 0, min_y = 0;

    if (needs_meta) {

        for (const auto& f : frames) {
            min_x = std::min(min_x, f.x);
            min_y = std::min(min_y, f.y);
        }
    }

    // build metadata frame pixels (if needed) before sizing the canvas
    std::vector<uint8_t> meta_pixels;
    uint16_t meta_w = 0;

    if (needs_meta) {
        meta_pixels = encode_offset_pixels(frames);
        meta_w = static_cast<uint16_t>(meta_pixels.size());
    }

    // compute canvas size using the visual (shifted) offsets
    uint16_t canvas_w = 0, canvas_h = 0;

    for (const auto& f : frames) {
        uint16_t vis_x = static_cast<uint16_t>(f.x - min_x);
        uint16_t vis_y = static_cast<uint16_t>(f.y - min_y);
        canvas_w = std::max(canvas_w, static_cast<uint16_t>(vis_x + f.w));
        canvas_h = std::max(canvas_h, static_cast<uint16_t>(vis_y + f.h));
    }

    // if adding a metadata frame, make room for it at the bottom
    if (needs_meta) {
        canvas_w = std::max(canvas_w, meta_w);
        canvas_h += 1;
    }

    // build palette array (16 colors, 3 bytes each)
    const auto& pal = frames[0].palette;
    uint8_t palette[16 * 3];

    for (int i = 0; i < 16; i++) {
        palette[i * 3]     = pal[i].r;
        palette[i * 3 + 1] = pal[i].g;
        palette[i * 3 + 2] = pal[i].b;
    }

    // depth=4 means 2^4=16 colors, bgindex=0, loop=0 (infinite)
    ge_GIF* gif = ge_new_gif(path.c_str(), canvas_w, canvas_h,
                             palette, 4, 0, 0);

    if (!gif) {
        throw std::runtime_error("cannot create GIF: " + path);
    }

    // write frames using visual (non-negative) offsets
    // use 10cs (100ms) delay per frame as a reasonable default.
    for (const auto& frame : frames) {
        uint16_t vis_x = static_cast<uint16_t>(frame.x - min_x);
        uint16_t vis_y = static_cast<uint16_t>(frame.y - min_y);
        ge_add_frame_at(gif, 10,
                        frame.pixels.data(),
                        vis_x, vis_y,
                        frame.w, frame.h);
    }

    // append metadata frame at the bottom of the canvas
    if (needs_meta) {
        meta_pixels.resize(canvas_w, 0);
        ge_add_frame_at(gif, 0,
                        meta_pixels.data(),
                        0, canvas_h - 1,
                        canvas_w, 1);
    }

    ge_close_gif(gif);
    return needs_meta;
}
