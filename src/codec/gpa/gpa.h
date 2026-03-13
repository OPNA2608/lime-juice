#pragma once

#include "image/image.h"

#include <cstdint>
#include <string>
#include <vector>

namespace gpa {

// standard gpa file header
inline constexpr const char* signature = "PC98)GPAFILE   ";

// decode a GPA animation file into a list of frames
std::vector<IndexedImage> decode(const std::vector<uint8_t>& data);

// encode a list of frames into GPA file bytes.
// palette is taken from the first frame.
std::vector<uint8_t> encode(const std::vector<IndexedImage>& frames);

} // namespace gpa
