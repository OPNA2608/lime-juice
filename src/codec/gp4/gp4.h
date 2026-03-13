#pragma once

#include "image/image.h"

#include <cstdint>
#include <vector>

namespace gp4 {

// decode a GP4 file from raw bytes into an IndexedImage.
// canvas_w overrides the internal canvas width (default 640).
// some games use a wider virtual framebuffer for scrolling scenes.
IndexedImage decode(const std::vector<uint8_t>& data, int canvas_w = 640);

// encode an IndexedImage into GP4 file bytes
std::vector<uint8_t> encode(const IndexedImage& img);

} // namespace gp4
