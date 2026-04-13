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

#include "../ast.h"
#include "../config.h"
#include <stdexcept>
#include <string>
#include <vector>

// opened MES file: dictionary entries + raw bytecode
struct MesFile {
    // dictionary: each entry is a pair of SJIS bytes
    std::vector<std::vector<int>> dictionary;
    // raw bytecode (from after the dictionary offset to end of file)
    std::vector<uint8_t> code;
};

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& msg, size_t pos)
        : std::runtime_error("parse error at position " + std::to_string(pos) + ": " + msg),
          position(pos) {}
    size_t position;
};
