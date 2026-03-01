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