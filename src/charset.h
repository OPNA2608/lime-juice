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
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// character encoding system for SJIS / Kuten / JIS / Unicode conversions
// translated from mes-charset.rkt

class Charset {
public:
    Charset();
    ~Charset();

    // reset all custom mappings
    void reset();

    // load a named charset (e.g. "pc98", "english")
    void load(const std::string& name);

    // --- custom mapping registration (used by charset data files) ---

    // register a single SJIS pair <-> Unicode mapping
    void register_mapping(const std::vector<int>& sjis_pair, char32_t unicode_char);

    // register a range of characters starting at Kuten position (ku, ten)
    // wraps around at column 94, incrementing row
    // this is the C++ equivalent of (charset* k t char1 char2 ...)
    void register_kuten_range(int ku, int ten, const std::vector<char32_t>& chars);

    // register a mapping from an SJIS integer to a Unicode char
    // converts the integer to a byte pair, then delegates to register_mapping
    // this is the C++ equivalent of (charset! sjis_int char)
    void register_sjis_mapping(int sjis_int, char32_t c);

    // fill count consecutive kuten positions with a single character
    // this is the C++ equivalent of (charset*** ku ten (* count 94) fill)
    void register_kuten_fill(int ku, int ten, int count, char32_t fill);

    // register a range from a UTF-8 encoded string (decodes to codepoints internally)
    // this is the C++ equivalent of (charset** ku ten "string...")
    void register_kuten_range_str(int ku, int ten, const char* utf8_str);

    // --- conversion functions ---

    // convert SJIS pair to Unicode character (custom tables first, then iconv fallback)
    // returns nullopt if conversion fails
    std::optional<char32_t> sjis_to_char(const std::vector<int>& sjis_pair) const;

    // convert Unicode character to SJIS pair (custom tables first, then iconv fallback)
    std::optional<std::vector<int>> char_to_sjis(char32_t ch) const;

    // --- arithmetic conversions (pure math, no tables) ---

    // kuten (row, col) -> SJIS byte pair
    static std::vector<int> kuten_to_sjis(int ku, int ten);

    // SJIS byte pair -> kuten (row, col)
    static std::pair<int, int> sjis_to_kuten(int j1, int j2);

    // kuten -> JIS
    static std::pair<int, int> kuten_to_jis(int ku, int ten);

    // JIS -> kuten
    static std::pair<int, int> jis_to_kuten(int j1, int j2);

    // SJIS pair -> single integer (j1*256 + j2, or j1 if j2==0)
    static int sjis_to_integer(const std::vector<int>& sjis_pair);

    // single integer -> SJIS pair
    static std::vector<int> integer_to_sjis(int i);

    // --- SJIS validation ---

    // check if SJIS code is in standard (non-PC98-exclusive) range
    static bool sjis_is_regular(int j1, int j2);

    // check if SJIS code falls in PC-98 exclusive section (kuten rows 9-15)
    static bool sjis_is_nonstandard(int j1, int j2);

    // --- special characters ---

    char32_t space_char() const { return char_space_; }
    char32_t newline_char() const { return char_newline_; }
    char32_t continue_char() const { return char_continue_; }
    char32_t break_char() const { return char_break_; }
    int fontwidth() const { return fontwidth_; }

    void set_space_char(char32_t c) { char_space_ = c; }
    void set_newline_char(char32_t c);
    void set_continue_char(char32_t c);
    void set_break_char(char32_t c);
    void set_fontwidth(int w) { fontwidth_ = w; }

private:
    // custom mapping tables
    std::unordered_map<uint32_t, char32_t> sjis_to_char_map_;
    std::unordered_map<char32_t, uint32_t> char_to_sjis_map_;

    // special characters
    char32_t char_space_    = U'\u3000';
    char32_t char_newline_  = U'\uFF05';  // fullwidth %
    char32_t char_continue_ = U'\uFF20';  // fullwidth @
    char32_t char_break_    = U'\uFF03';  // fullwidth #
    int fontwidth_ = 2;

    // helper: pack SJIS pair into a single key for hash lookup
    static uint32_t sjis_key(int j1, int j2) {
        return (static_cast<uint32_t>(j1) << 8) | static_cast<uint32_t>(j2);
    }

    // iconv-based SJIS <-> UTF-8 fallback (optional, not available on all platforms)
    std::optional<char32_t> sjis_to_char_iconv(int sjis_int) const;
    std::optional<std::vector<int>> char_to_sjis_iconv(char32_t ch) const;

#ifdef HAVE_ICONV
    // iconv handles (lazily initialized)
    mutable void* iconv_sjis_to_utf8_ = nullptr;
    mutable void* iconv_utf8_to_sjis_ = nullptr;
    void ensure_iconv() const;
    void close_iconv();
#endif
};

// --- charset data registration functions (called by load()) ---
void register_charset_pc98(Charset& cs);
void register_charset_english(Charset& cs);
void register_charset_europe(Charset& cs);
void register_charset_korean(Charset& cs);
void register_charset_korean_gamebox(Charset& cs);
void register_charset_korean_hannuri(Charset& cs);
void register_charset_korean_kk(Charset& cs);
void register_charset_korean_parangsae(Charset& cs);
void register_charset_chinese(Charset& cs);
