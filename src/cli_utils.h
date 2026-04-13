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

#include <cstdint>
#include <string>
#include <vector>

// ansi color printing
void print_color(const std::string& color, const std::string& text);
void println_color(const std::string& color, const std::string& text);

// enable ANSI escape sequences on Windows (no-op on other platforms)
void enable_ansi_console();

// path utilities
bool has_extension(const std::string& path, const std::string& ext);
std::string replace_ext(const std::string& path, const std::string& new_ext);
std::string get_ext(const std::string& path);
std::string get_stem(const std::string& path);

// file I/O
std::vector<uint8_t> read_file(const std::string& path);
void write_file(const std::string& path, const std::vector<uint8_t>& data);

// expand glob patterns (* and ?) in file arguments
std::vector<std::string> expand_globs(const std::vector<std::string>& args);
