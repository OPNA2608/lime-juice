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
