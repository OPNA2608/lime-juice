#pragma once

#include <stdexcept>
#include <string>

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& msg, size_t pos)
        : std::runtime_error("parse error at position " + std::to_string(pos) + ": " + msg),
          position(pos) {}
    size_t position;
};
