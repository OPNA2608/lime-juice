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
