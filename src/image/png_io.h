#pragma once

#include "image/image.h"
#include <string>

// load a PNG file into an IndexedImage.
// reads tEXt chunks "juice-x" and "juice-y" for screen position.
// input PNG must be indexed (palette) color type with <= 16 colors.
IndexedImage load_png(const std::string& path);

// save an IndexedImage to a PNG file (4-bit indexed color).
// writes tEXt chunks "juice-x" and "juice-y" for screen position.
void save_png(const std::string& path, const IndexedImage& img);
