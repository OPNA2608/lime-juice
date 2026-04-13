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
#include <memory>
#include <string>
#include <variant>
#include <vector>

// ast node for the mes decompiler
// maps to the racket s-expression representation used throughout juice

struct AstNode {
    enum class Kind {
        Integer,    // numeric literal
        Variable,   // variable reference (A-Z)
        Symbol,     // named symbol (opcode names, keywords)
        String,     // string literal / text content
        Character,  // decoded unicode character
        CharRaw,    // undecoded SJIS byte pair
        DicRef,     // dictionary reference index
        List,       // compound node: tag + children
        Cut,        // (cut) marker for menu blocks
        Keyword,    // racket keyword like #:color
        Boolean,    // #t or #f
        Quote,      // quoted value '(...)
    };

    Kind kind;

    // atom values (valid depending on kind)
    int32_t int_val = 0;
    char var_val = '\0';
    std::string str_val;
    char32_t char_val = 0;
    std::vector<uint8_t> raw_bytes;
    bool bool_val = false;

    // list fields
    std::string tag;
    std::vector<AstNode> children;

    // --- factory methods ---

    static AstNode make_integer(int32_t n) {
        AstNode node;
        node.kind = Kind::Integer;
        node.int_val = n;
        return node;
    }

    static AstNode make_variable(char v) {
        AstNode node;
        node.kind = Kind::Variable;
        node.var_val = v;
        return node;
    }

    static AstNode make_symbol(const std::string& name) {
        AstNode node;
        node.kind = Kind::Symbol;
        node.str_val = name;
        return node;
    }

    static AstNode make_string(const std::string& s) {
        AstNode node;
        node.kind = Kind::String;
        node.str_val = s;
        return node;
    }

    static AstNode make_character(char32_t c) {
        AstNode node;
        node.kind = Kind::Character;
        node.char_val = c;
        return node;
    }

    static AstNode make_char_raw(uint8_t j1, uint8_t j2) {
        AstNode node;
        node.kind = Kind::CharRaw;
        node.raw_bytes = {j1, j2};
        return node;
    }

    static AstNode make_dic_ref(int index) {
        AstNode node;
        node.kind = Kind::DicRef;
        node.int_val = index;
        return node;
    }

    static AstNode make_list(const std::string& tag_name, std::vector<AstNode> kids = {}) {
        AstNode node;
        node.kind = Kind::List;
        node.tag = tag_name;
        node.children = std::move(kids);
        return node;
    }

    static AstNode make_cut() {
        AstNode node;
        node.kind = Kind::Cut;
        return node;
    }

    static AstNode make_keyword(const std::string& kw) {
        AstNode node;
        node.kind = Kind::Keyword;
        node.str_val = kw;
        return node;
    }

    static AstNode make_boolean(bool b) {
        AstNode node;
        node.kind = Kind::Boolean;
        node.bool_val = b;
        return node;
    }

    static AstNode make_quote(AstNode inner) {
        AstNode node;
        node.kind = Kind::Quote;
        node.children.push_back(std::move(inner));
        return node;
    }

    // --- convenience checks ---

    bool is_list() const { return kind == Kind::List; }

    bool is_list(const std::string& t) const {
        return kind == Kind::List && tag == t;
    }

    bool is_integer() const { return kind == Kind::Integer; }
    bool is_variable() const { return kind == Kind::Variable; }
    bool is_symbol() const { return kind == Kind::Symbol; }
    bool is_string() const { return kind == Kind::String; }
    bool is_character() const { return kind == Kind::Character; }
    bool is_char_raw() const { return kind == Kind::CharRaw; }
    bool is_dic_ref() const { return kind == Kind::DicRef; }
    bool is_cut() const { return kind == Kind::Cut; }
    bool is_keyword() const { return kind == Kind::Keyword; }
    bool is_boolean() const { return kind == Kind::Boolean; }
    bool is_quote() const { return kind == Kind::Quote; }

    bool is_symbol(const std::string& name) const {
        return kind == Kind::Symbol && str_val == name;
    }

    // --- equality ---

    bool operator==(const AstNode& other) const;
    bool operator!=(const AstNode& other) const { return !(*this == other); }
};
