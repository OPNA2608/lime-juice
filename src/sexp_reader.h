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
