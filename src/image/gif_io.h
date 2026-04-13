//
// lime-juice: C++ port of Tomyun's "Juice" de/recompiler for PC-98 games
// Copyright (C) 2026 Fuzion
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

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
