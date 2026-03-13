#pragma once

#include "image/image.h"

#include <cstddef>
#include <cstdint>
#include <vector>

// shared GPC compression primitives used by both GPC and GPA codecs

namespace gpc {
namespace internal {

// decode compressed frame data into pixel indices.
// reads from data starting at pos (pos is updated).
// w, h: frame pixel dimensions.
// interleaving: vertical interlace value.
// returns w*h pixel indices (0-15), row-major.
std::vector<uint8_t> decode_frame_data(const std::vector<uint8_t>& data,
                                       size_t& pos, int w, int h,
                                       int interleaving);

struct CompressedFrame {
    std::vector<uint8_t> data;
    int interleaving;
};

// encode pixel data into compressed frame bytes.
// pixels: w*h palette indices (0-15), row-major.
// returns compressed data and chosen interleave value.
CompressedFrame encode_frame_data(const std::vector<uint8_t>& pixels,
                                  int w, int h);

} // namespace internal
} // namespace gpc
