#pragma once

#include "image/image.h"

#include <string>
#include <vector>

// load a GIF file into a list of frames.
// each frame gets per-frame x/y offset, dimensions, and pixel indices.
// palette is taken from the GIF's global or first local palette.
std::vector<IndexedImage> load_gif(const std::string& path);

// save a list of frames as a GIF89a animation.
// palette is taken from the first frame.
// canvas size is computed as the bounding box of all frames.
// returns true if a metadata frame was appended (negative offsets).
bool save_gif(const std::string& path,
              const std::vector<IndexedImage>& frames);
