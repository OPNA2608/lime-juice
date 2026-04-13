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

#include "ast.h"
#include <string>
#include <stdexcept>

// s-expression parser
// inverse of SexpWriter: reads text produced by SexpWriter back into AstNode trees

class SexpReader {
public:
    // parse a complete s-expression from text
    AstNode parse(const std::string& input);

private:
    const char* pos = nullptr;
    const char* end = nullptr;
    int line = 1;
    int col = 1;

    // skip whitespace and ; comments
    void skip_ws();

    // read a single datum (atom, list, quote, etc.)
    AstNode read_datum();

    // read a list: (tag children...) or (child1 child2 ...)
    AstNode read_list();

    // read a string literal: "text with \"escapes\""
    std::string read_string_literal();

    // read a hash-prefixed form: #t, #f, #\char, #:keyword
    AstNode read_hash();

    // read a character literal after #\ (e.g. newline, space, uXXXX, single char)
    AstNode read_char_literal();

    // read a quote: 'datum
    AstNode read_quote();

    // read a number (possibly negative) or symbol/identifier
    AstNode read_atom();

    // helpers
    char peek() const;
    char advance();
    bool at_end() const;

    // check if a character can start or continue an identifier/symbol
    static bool is_ident_start(char c);
    static bool is_ident_char(char c);

    // check if a string is a single uppercase variable letter
    static bool is_variable(const std::string& s);

    [[noreturn]] void error(const std::string& msg) const;
};
