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
