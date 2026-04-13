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

// s-expression pretty printer
// produces output matching racket's (pretty-format) with the juice style table
// translated from juice.rkt lines 119-125

class SexpWriter {
public:
    // format an AST node as a pretty-printed s-expression string
    std::string format(const AstNode& node);

private:
    // formatting modes based on racket's style table
    enum class Style {
        Default,    // greedy word-wrap: pack children until line width exceeded
        Block,      // structural containers: one child per line
        Define,     // define-proc: 2-column layout (name on first line)
        If,         // if, if-else, cond, while, slot, seg: 2-column
        Set,        // set-arr~, set-reg:, set-var, etc.: 2-column
        Inline,     // text, str: always single line regardless of width
    };

    Style get_style(const std::string& tag) const;

    void write_node(std::string& out, const AstNode& node, int indent);
    void write_list(std::string& out, const AstNode& node, int indent);
    void write_atom(std::string& out, const AstNode& node);

    // write a character literal in racket format (#\a, #\newline, etc.)
    void write_char_literal(std::string& out, char32_t c);

    // estimate the printed length of a node (for line-break decisions)
    int estimate_width(const AstNode& node);

    static constexpr int MAX_LINE_WIDTH = 80;
};
