#pragma once

#include "image/image.h"

#include <cstdint>
#include <vector>

namespace gpc {

// standard GPC file signature
inline constexpr const char* signature = "PC98)GPCFILE   ";

// decode a GPC file from raw bytes into an IndexedImage
IndexedImage decode(const std::vector<uint8_t>& data);

// encode an IndexedImage into GPC file bytes
std::vector<uint8_t> encode(const IndexedImage& img);

} // namespace gpc
