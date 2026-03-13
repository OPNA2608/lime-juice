#include "image/png_io.h"

#include <lodepng/lodepng.h>

#include <cstring>
#include <fstream>
#include <stdexcept>

IndexedImage load_png(const std::string& path) {

    // read file into memory
    std::ifstream file(path, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("cannot open file: " + path);
    }

    std::vector<uint8_t> png_data(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    LodePNGState state;
    lodepng_state_init(&state);

    // decode as raw palette indices (no color conversion)
    state.decoder.color_convert = 0;
    state.decoder.read_text_chunks = 1;

    unsigned char* raw = nullptr;
    unsigned w = 0, h = 0;
    unsigned err = lodepng_decode(&raw, &w, &h, &state,
                                  png_data.data(), png_data.size());

    if (err) {
        lodepng_state_cleanup(&state);
        free(raw);
        throw std::runtime_error(
            "PNG decode error: " + std::string(lodepng_error_text(err)));
    }

    // verify the PNG is palette-based
    if (state.info_png.color.colortype != LCT_PALETTE) {
        lodepng_state_cleanup(&state);
        free(raw);
        throw std::runtime_error("PNG must be indexed (palette) color type");
    }

    if (state.info_png.color.palettesize > 16) {
        lodepng_state_cleanup(&state);
        free(raw);
        throw std::runtime_error("PNG palette must have 16 or fewer colors");
    }

    IndexedImage img;
    img.w = static_cast<uint16_t>(w);
    img.h = static_cast<uint16_t>(h);

    // extract palette (lodepng stores as RGBA)
    for (size_t i = 0; i < state.info_png.color.palettesize && i < 16; i++) {
        img.palette[i].r = state.info_png.color.palette[i * 4 + 0];
        img.palette[i].g = state.info_png.color.palette[i * 4 + 1];
        img.palette[i].b = state.info_png.color.palette[i * 4 + 2];
    }

    // extract pixel indices.
    // with color_convert=0, lodepng gives us raw indices at the source bitdepth.
    // for 4-bit PNGs, each byte holds two pixels (high nibble first).
    // for 8-bit PNGs, each byte is one index.
    unsigned bitdepth = state.info_png.color.bitdepth;
    img.pixels.resize(w * h);

    if (bitdepth == 8) {
        std::memcpy(img.pixels.data(), raw, w * h);
    } else if (bitdepth == 4) {
        // each byte holds 2 pixels, high nibble first.
        // lodepng returns flat-packed data (no per-row byte padding),
        // so we use the global pixel index for nibble extraction.
        for (unsigned y = 0; y < h; y++) {

            for (unsigned x = 0; x < w; x++) {
                size_t px_idx = static_cast<size_t>(y) * w + x;
                uint8_t byte = raw[px_idx / 2];

                if (px_idx % 2 == 0) {
                    img.pixels[px_idx] = (byte >> 4) & 0x0F;
                } else {
                    img.pixels[px_idx] = byte & 0x0F;
                }
            }
        }
    } else if (bitdepth <= 2) {
        // 1-bit or 2-bit palette.
        // lodepng returns flat-packed data (no per-row byte padding),
        // so we use the global pixel index for bit extraction.
        unsigned ppb = 8 / bitdepth;

        for (unsigned y = 0; y < h; y++) {

            for (unsigned x = 0; x < w; x++) {
                size_t px_idx = static_cast<size_t>(y) * w + x;
                uint8_t byte = raw[px_idx / ppb];
                unsigned shift = (ppb - 1 - (px_idx % ppb)) * bitdepth;
                img.pixels[px_idx] = (byte >> shift) & ((1 << bitdepth) - 1);
            }
        }
    } else {
        lodepng_state_cleanup(&state);
        free(raw);
        throw std::runtime_error("unsupported palette bit depth: " +
                                  std::to_string(bitdepth));
    }

    // read tEXt chunks for screen position
    for (size_t i = 0; i < state.info_png.text_num; i++) {

        if (std::strcmp(state.info_png.text_keys[i], "juice-x") == 0) {
            img.x = static_cast<int16_t>(std::stoi(state.info_png.text_strings[i]));
        } else if (std::strcmp(state.info_png.text_keys[i], "juice-y") == 0) {
            img.y = static_cast<int16_t>(std::stoi(state.info_png.text_strings[i]));
        }
    }

    free(raw);
    lodepng_state_cleanup(&state);
    return img;
}

void save_png(const std::string& path, const IndexedImage& img) {
    LodePNGState state;
    lodepng_state_init(&state);

    // configure output as 4-bit indexed PNG
    state.info_raw.colortype = LCT_PALETTE;
    state.info_raw.bitdepth = 4;
    state.info_png.color.colortype = LCT_PALETTE;
    state.info_png.color.bitdepth = 4;
    state.encoder.auto_convert = 0;

    // set palette (as RGBA with alpha = 255)
    for (size_t i = 0; i < 16; i++) {
        lodepng_palette_add(&state.info_png.color,
                            img.palette[i].r, img.palette[i].g,
                            img.palette[i].b, 255);
        lodepng_palette_add(&state.info_raw,
                            img.palette[i].r, img.palette[i].g,
                            img.palette[i].b, 255);
    }

    // add tEXt chunks for screen position
    lodepng_add_text(&state.info_png, "juice-x", std::to_string(img.x).c_str());
    lodepng_add_text(&state.info_png, "juice-y", std::to_string(img.y).c_str());

    // pack pixels into 4-bit format (high nibble first).
    // lodepng expects flat-packed data (no per-row byte padding),
    // so we use the global pixel index for nibble placement.
    size_t total_pixels = static_cast<size_t>(img.w) * img.h;
    std::vector<uint8_t> raw((total_pixels + 1) / 2, 0);

    for (unsigned y = 0; y < img.h; y++) {

        for (unsigned x = 0; x < img.w; x++) {
            size_t px_idx = static_cast<size_t>(y) * img.w + x;
            uint8_t idx = img.pixels[px_idx] & 0x0F;

            if (px_idx % 2 == 0) {
                raw[px_idx / 2] |= (idx << 4);
            } else {
                raw[px_idx / 2] |= idx;
            }
        }
    }

    unsigned char* out = nullptr;
    size_t outsize = 0;
    unsigned err = lodepng_encode(&out, &outsize, raw.data(),
                                  img.w, img.h, &state);

    if (err) {
        free(out);
        lodepng_state_cleanup(&state);
        throw std::runtime_error(
            "PNG encode error: " + std::string(lodepng_error_text(err)));
    }

    // write to file
    std::ofstream file(path, std::ios::binary);

    if (!file.is_open()) {
        free(out);
        lodepng_state_cleanup(&state);
        throw std::runtime_error("cannot write file: " + path);
    }

    file.write(reinterpret_cast<const char*>(out), outsize);
    free(out);
    lodepng_state_cleanup(&state);
}
