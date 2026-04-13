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

#include "sexp_reader.h"

#include <cctype>
#include <cstdlib>
#include <sstream>

AstNode SexpReader::parse(const std::string& input) {
    pos = input.data();
    end = input.data() + input.size();
    line = 1;
    col = 1;

    skip_ws();

    if (at_end()) {
        error("unexpected end of input");
    }

    AstNode result = read_datum();

    skip_ws();

    if (!at_end()) {
        error("trailing content after top-level expression");
    }

    return result;
}

// ── helpers ──────────────────────────────────────────────────────────

char SexpReader::peek() const {
    if (at_end()) {
        return '\0';
    }

    return *pos;
}

char SexpReader::advance() {
    if (at_end()) {
        error("unexpected end of input");
    }

    char c = *pos++;

    if (c == '\n') {
        line++;
        col = 1;
    } else {
        col++;
    }

    return c;
}

bool SexpReader::at_end() const {
    return pos >= end;
}

bool SexpReader::is_ident_start(char c) {
    // identifiers can start with letters, and various special characters
    // used in tag names and operators across all engines:
    // <, >, /, +, *, ?, =, -, ~, :, !, @, ^, &, |, %
    // high bytes (>= 0x80) are also valid for undecoded SJIS/UTF-8 chars
    unsigned char uc = static_cast<unsigned char>(c);

    if (uc >= 0x80) {
        return true;
    }

    if (std::isalpha(uc)) {
        return true;
    }

    switch (c) {
        case '<': case '>': case '/': case '+': case '*':
        case '?': case '=': case '-': case '~': case ':':
        case '!': case '@': case '^': case '_':
        case '&': case '|': case '%':
            return true;
        default:
            return false;
    }
}

bool SexpReader::is_ident_char(char c) {
    if (is_ident_start(c)) {
        return true;
    }

    // digits and a few more chars are valid inside identifiers
    if (std::isdigit(static_cast<unsigned char>(c))) {
        return true;
    }

    switch (c) {
        case '.': case '$':
            return true;
        default:
            return false;
    }
}

bool SexpReader::is_variable(const std::string& s) {
    return s.size() == 1 && s[0] >= 'A' && s[0] <= 'Z';
}

void SexpReader::error(const std::string& msg) const {
    std::ostringstream oss;
    oss << "sexp parse error at line " << line << " col " << col << ": " << msg;
    throw std::runtime_error(oss.str());
}

// ── whitespace / comment skipping ────────────────────────────────────

void SexpReader::skip_ws() {
    while (!at_end()) {
        char c = peek();

        if (c == ';') {
            // skip comment to end of line
            while (!at_end() && peek() != '\n') {
                advance();
            }

            continue;
        }

        if (std::isspace(static_cast<unsigned char>(c))) {
            advance();
            continue;
        }

        break;
    }
}

// ── main dispatch ────────────────────────────────────────────────────

AstNode SexpReader::read_datum() {
    skip_ws();

    if (at_end()) {
        error("unexpected end of input");
    }

    char c = peek();

    if (c == '(') {
        return read_list();
    }

    if (c == '"') {
        std::string s = read_string_literal();
        return AstNode::make_string(s);
    }

    if (c == '#') {
        return read_hash();
    }

    if (c == '\'') {
        return read_quote();
    }

    return read_atom();
}

// ── list parsing ─────────────────────────────────────────────────────

AstNode SexpReader::read_list() {
    advance(); // consume '('
    skip_ws();

    // empty parens: ()
    if (peek() == ')') {
        advance();
        return AstNode::make_list("");
    }

    // peek at first element to decide if it's a tag or a child
    char c = peek();
    bool has_tag = false;
    std::string tag;

    // if first element starts with an identifier character (not a digit,
    // not (, not ", not #, not '), treat it as the tag
    if (is_ident_start(c) && c != '#') {
        // it could be a negative number: -123
        // check if it's '-' followed by a digit
        if (c == '-' && (pos + 1 < end) && std::isdigit(static_cast<unsigned char>(*(pos + 1)))) {
            has_tag = false;
        } else {
            has_tag = true;

            // read the tag as an identifier
            const char* start = pos;

            while (!at_end() && is_ident_char(peek())) {
                advance();
            }

            tag = std::string(start, pos);
        }
    }

    // read children
    std::vector<AstNode> children;

    if (!has_tag) {
        // first element is a child, parse it as a datum
        children.push_back(read_datum());
    }

    skip_ws();

    while (!at_end() && peek() != ')') {
        children.push_back(read_datum());
        skip_ws();
    }

    if (at_end()) {
        error("unterminated list");
    }

    advance(); // consume ')'

    // special forms that look like lists but produce specific AstNode kinds
    if (has_tag) {

        if (tag == "cut" && children.empty()) {
            return AstNode::make_cut();
        }

        if (tag == "chr-raw" && children.size() == 2 &&
            children[0].is_integer() && children[1].is_integer()) {
            return AstNode::make_char_raw(
                static_cast<uint8_t>(children[0].int_val),
                static_cast<uint8_t>(children[1].int_val));
        }

        if (tag == "dic" && children.size() == 1 && children[0].is_integer()) {
            return AstNode::make_dic_ref(children[0].int_val);
        }
    }

    AstNode node = AstNode::make_list(tag, std::move(children));
    return node;
}

// ── string literal ───────────────────────────────────────────────────

std::string SexpReader::read_string_literal() {
    advance(); // consume opening "
    std::string result;

    while (!at_end() && peek() != '"') {
        char c = advance();

        if (c == '\\') {

            if (at_end()) {
                error("unterminated escape in string");
            }

            char esc = advance();

            switch (esc) {
                case '"':  result += '"'; break;
                case '\\': result += '\\'; break;
                case 'n':  result += '\n'; break;
                case 't':  result += '\t'; break;
                default:
                    // preserve unknown escapes
                    result += '\\';
                    result += esc;
                    break;
            }
        } else {
            result += c;
        }
    }

    if (at_end()) {
        error("unterminated string");
    }

    advance(); // consume closing "
    return result;
}

// ── hash-prefixed forms ──────────────────────────────────────────────

AstNode SexpReader::read_hash() {
    advance(); // consume '#'

    if (at_end()) {
        error("unexpected end after #");
    }

    char c = peek();

    // booleans
    if (c == 't') {
        advance();
        return AstNode::make_boolean(true);
    }

    if (c == 'f') {
        advance();
        return AstNode::make_boolean(false);
    }

    // character literal
    if (c == '\\') {
        advance(); // consume '\'
        return read_char_literal();
    }

    // byte-string literal #"..." (racket format, contains raw bytes and octal escapes)
    if (c == '"') {
        advance(); // consume opening "
        std::string result;

        while (!at_end() && peek() != '"') {
            char ch = advance();

            if (ch == '\\') {

                if (at_end()) {
                    error("unterminated escape in byte-string");
                }

                // check for octal escape: \NNN
                if (std::isdigit(static_cast<unsigned char>(peek()))) {
                    int val = 0;

                    for (int i = 0; i < 3 && !at_end() &&
                         std::isdigit(static_cast<unsigned char>(peek())); i++) {
                        val = val * 8 + (advance() - '0');
                    }

                    result += static_cast<char>(val);
                } else {
                    char esc = advance();

                    switch (esc) {
                        case '\\': result += '\\'; break;
                        case '"':  result += '"'; break;
                        case 'n':  result += '\n'; break;
                        case 't':  result += '\t'; break;
                        default:
                            result += '\\';
                            result += esc;
                            break;
                    }
                }
            } else {
                result += ch;
            }
        }

        if (at_end()) {
            error("unterminated byte-string");
        }

        advance(); // consume closing "
        return AstNode::make_string(result);
    }

    // keyword
    if (c == ':') {
        advance(); // consume ':'
        const char* start = pos;

        while (!at_end() && is_ident_char(peek())) {
            advance();
        }

        std::string kw(start, pos);

        if (kw.empty()) {
            error("empty keyword after #:");
        }

        return AstNode::make_keyword(kw);
    }

    error(std::string("unexpected character after #: ") + c);
}

// ── character literal after #\ ───────────────────────────────────────

AstNode SexpReader::read_char_literal() {
    if (at_end()) {
        error("unexpected end after #\\");
    }

    // check for unicode escape: #\uXXXX (must come before alpha identifier check)
    if (peek() == 'u') {
        advance(); // consume 'u'
        const char* hex_start = pos;

        while (!at_end() && std::isxdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }

        std::string hex_str(hex_start, pos);

        if (hex_str.empty()) {
            // no hex digits after 'u': just the character 'u'
            return AstNode::make_character(U'u');
        }

        char32_t code = static_cast<char32_t>(std::stoul(hex_str, nullptr, 16));
        return AstNode::make_character(code);
    }

    // check for named characters
    const char* start = pos;

    // try reading an identifier-like name (for newline, space, tab, etc.)
    if (std::isalpha(static_cast<unsigned char>(peek()))) {
        while (!at_end() && std::isalpha(static_cast<unsigned char>(peek()))) {
            advance();
        }

        std::string name(start, pos);

        // named character constants
        if (name == "newline")   { return AstNode::make_character('\n'); }
        if (name == "space")     { return AstNode::make_character(' '); }
        if (name == "tab")       { return AstNode::make_character('\t'); }
        if (name == "backspace") { return AstNode::make_character('\b'); }

        // single ascii letter: #\a is just the char 'a'
        if (name.size() == 1) {
            return AstNode::make_character(static_cast<char32_t>(name[0]));
        }

        error("unknown character name: " + name);
    }

    // single non-alpha character: read as UTF-8 codepoint
    // UTF-8 decoding: determine how many bytes this character spans
    unsigned char first = static_cast<unsigned char>(advance());
    char32_t codepoint;

    if (first < 0x80) {
        codepoint = first;
    } else if ((first & 0xE0) == 0xC0) {

        if (at_end()) { error("incomplete UTF-8 in character literal"); }

        unsigned char b2 = static_cast<unsigned char>(advance());
        codepoint = ((first & 0x1F) << 6) | (b2 & 0x3F);
    } else if ((first & 0xF0) == 0xE0) {

        if (pos + 1 >= end) { error("incomplete UTF-8 in character literal"); }

        unsigned char b2 = static_cast<unsigned char>(advance());
        unsigned char b3 = static_cast<unsigned char>(advance());
        codepoint = ((first & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
    } else if ((first & 0xF8) == 0xF0) {

        if (pos + 2 >= end) { error("incomplete UTF-8 in character literal"); }

        unsigned char b2 = static_cast<unsigned char>(advance());
        unsigned char b3 = static_cast<unsigned char>(advance());
        unsigned char b4 = static_cast<unsigned char>(advance());
        codepoint = ((first & 0x07) << 18) | ((b2 & 0x3F) << 12) |
                    ((b3 & 0x3F) << 6) | (b4 & 0x3F);
    } else {
        error("invalid UTF-8 byte in character literal");
    }

    return AstNode::make_character(codepoint);
}

// ── quote ────────────────────────────────────────────────────────────

AstNode SexpReader::read_quote() {
    advance(); // consume '

    // the SexpWriter writes ' immediately followed by the datum (no space).
    // if ' is followed by whitespace, it's a bare quote with no children,
    // as seen in sound commands: (sound ' 2) where ' and 2 are separate.
    if (at_end() || peek() == ')' ||
        std::isspace(static_cast<unsigned char>(peek()))) {
        // bare quote: no children
        AstNode node;
        node.kind = AstNode::Kind::Quote;
        return node;
    }

    AstNode inner = read_datum();
    return AstNode::make_quote(std::move(inner));
}

// ── atoms (numbers, symbols, variables) ──────────────────────────────

AstNode SexpReader::read_atom() {
    const char* start = pos;
    char c = peek();

    // negative number: starts with - followed by digit
    if (c == '-' && (pos + 1 < end) && std::isdigit(static_cast<unsigned char>(*(pos + 1)))) {
        advance(); // consume '-'

        while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }

        std::string num_str(start, pos);
        int32_t val = static_cast<int32_t>(std::stol(num_str));
        return AstNode::make_integer(val);
    }

    // positive number
    if (std::isdigit(static_cast<unsigned char>(c))) {

        while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }

        std::string num_str(start, pos);
        int32_t val = static_cast<int32_t>(std::stol(num_str));
        return AstNode::make_integer(val);
    }

    // pipe-quoted symbol: |...|
    // racket uses this for symbols containing special characters like [ or ]
    if (c == '|') {
        advance(); // consume opening |
        std::string sym;

        while (!at_end() && peek() != '|') {
            sym += advance();
        }

        if (at_end()) {
            error("unterminated pipe-quoted symbol");
        }

        advance(); // consume closing |
        return AstNode::make_symbol(sym);
    }

    // identifier / symbol / variable
    if (is_ident_start(c)) {

        while (!at_end() && is_ident_char(peek())) {
            advance();
        }

        std::string ident(start, pos);

        // single uppercase letter is a variable
        if (is_variable(ident)) {
            return AstNode::make_variable(ident[0]);
        }

        return AstNode::make_symbol(ident);
    }

    error(std::string("unexpected character: ") + c);
}
