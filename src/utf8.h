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

#include <string>

// encode a single Unicode codepoint as a UTF-8 string
inline std::string char32_to_utf8(char32_t c) {
    std::string result;

    if (c < 0x80) {
        result += static_cast<char>(c);
    } else if (c < 0x800) {
        result += static_cast<char>(0xC0 | (c >> 6));
        result += static_cast<char>(0x80 | (c & 0x3F));
    } else if (c < 0x10000) {
        result += static_cast<char>(0xE0 | (c >> 12));
        result += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (c & 0x3F));
    } else {
        result += static_cast<char>(0xF0 | (c >> 18));
        result += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
        result += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (c & 0x3F));
    }

    return result;
}
