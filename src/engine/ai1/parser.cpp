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

#include "parser.h"

namespace ai1 {

Parser::Parser(ByteStream& stream, const Config& cfg)
    : stream_(stream), cfg_(cfg) {}

// --- lexer functions ---

AstNode Parser::lex_reg0() {
    // REG0: 0x01-0x07, value = byte - 1
    uint8_t b = stream_.consume();
    int value = b - 0x01;
    return AstNode::make_list(":", {
        AstNode::make_list("num", {AstNode::make_integer(value)})
    });
}

AstNode Parser::lex_reg1() {
    // REG1: 0x00 + 1 byte
    stream_.consume(); // consume 0x00
    uint8_t c = stream_.consume();
    return AstNode::make_list(":", {
        AstNode::make_list("num", {AstNode::make_integer(c)})
    });
}

AstNode Parser::lex_reg2() {
    // REG2: 0x08 + 2 bytes
    stream_.consume(); // consume 0x08
    uint8_t c1 = stream_.consume();
    uint8_t c2 = stream_.consume();
    int value = (c1 << 8) | c2;
    return AstNode::make_list(":", {
        AstNode::make_list("num", {AstNode::make_integer(value)})
    });
}

AstNode Parser::lex_num0() {
    // NUM0: 0x11-0x17, value = byte - 0x11
    uint8_t b = stream_.consume();
    return AstNode::make_list("num", {AstNode::make_integer(b - 0x11)});
}

AstNode Parser::lex_num1() {
    // NUM1: 0x10 + 1 byte
    stream_.consume(); // consume 0x10
    uint8_t c = stream_.consume();
    return AstNode::make_list("num", {AstNode::make_integer(c)});
}

AstNode Parser::lex_num2() {
    // NUM2: 0x18 + 2 bytes
    stream_.consume(); // consume 0x18
    uint8_t c1 = stream_.consume();
    uint8_t c2 = stream_.consume();
    int value = (c1 << 8) | c2;
    return AstNode::make_list("num", {AstNode::make_integer(value)});
}

std::string Parser::lex_str() {
    // STR: 0x22 + chars + 0x22
    stream_.consume(); // consume opening 0x22
    std::string result;

    while (!stream_.at_end()) {
        uint8_t b = stream_.peek();

        if (b == TOK_STR) {
            stream_.consume(); // consume closing 0x22
            return result;
        }

        // valid string chars: 0x20-0x21, 0x23-0x7E, 0x80-0xA0, 0xA1-0xDF
        if ((b >= 0x20 && b <= 0x7E) || (b >= 0x80 && b <= 0xDF)) {
            stream_.consume();

            if (b >= 0xA1 && b <= 0xDF) {
                // JIS X 0201 half-width katakana → Unicode U+FF61-U+FF9F
                char32_t cp = static_cast<char32_t>(b) + 0xFEC0;
                result += static_cast<char>(0xE0 | (cp >> 12));
                result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (cp & 0x3F));
            } else if (b >= 0x80) {
                // other high bytes (0x80-0xA0): encode as UTF-8 Latin-1
                result += static_cast<char>(0xC0 | (b >> 6));
                result += static_cast<char>(0x80 | (b & 0x3F));
            } else {
                result += static_cast<char>(b);
            }
        } else {
            throw ParseError("unexpected byte in string: " + std::to_string(b), stream_.position());
        }
    }

    throw ParseError("unterminated string", stream_.position());
}

AstNode Parser::lex_chr() {
    // CHR: SJIS character pair
    uint8_t c1 = stream_.consume(); // 0x80-0x98
    uint8_t c2 = stream_.consume(); // 0x40-0xFC
    return AstNode::make_list("chr-raw", {
        AstNode::make_integer(c1),
        AstNode::make_integer(c2)
    });
}

AstNode Parser::lex_proc() {
    // PROC: 0xC0-0xFF, value = byte - 0xC0
    uint8_t b = stream_.consume();
    return AstNode::make_list("proc", {AstNode::make_integer(b - 0xC0)});
}

// --- grammar rules ---

AstNode Parser::parse_reg() {
    uint8_t b = stream_.peek();

    if (b >= 0x01 && b <= 0x07) {
        return lex_reg0();
    }

    if (b == TOK_REG1) {
        return lex_reg1();
    }

    if (b == TOK_REG2) {
        return lex_reg2();
    }

    throw ParseError("expected register token", stream_.position());
}

AstNode Parser::parse_num() {
    uint8_t b = stream_.peek();

    if (b >= 0x11 && b <= 0x17) {
        return lex_num0();
    }

    if (b == TOK_NUM1) {
        return lex_num1();
    }

    if (b == TOK_NUM2) {
        return lex_num2();
    }

    throw ParseError("expected number token", stream_.position());
}

AstNode Parser::parse_term() {
    uint8_t b = stream_.peek();

    // REG0: 0x01-0x07
    if (b >= 0x01 && b <= 0x07) {
        return lex_reg0();
    }

    // REG1: 0x00
    if (b == TOK_REG1) {
        return lex_reg1();
    }

    // REG2: 0x08
    if (b == TOK_REG2) {
        return lex_reg2();
    }

    // NUM0: 0x11-0x17
    if (b >= 0x11 && b <= 0x17) {
        return lex_num0();
    }

    // NUM1: 0x10
    if (b == TOK_NUM1) {
        return lex_num1();
    }

    // NUM2: 0x18
    if (b == TOK_NUM2) {
        return lex_num2();
    }

    // VAR: 0x40-0x5A
    if (b >= 0x40 && b <= 0x5A) {
        stream_.consume();
        return AstNode::make_list("var", {AstNode::make_integer(b)});
    }

    // TERM0: 0x3F (?)
    if (b == 0x3F) {
        stream_.consume();
        return AstNode::make_list("term0", {AstNode::make_integer(b)});
    }

    // TERM2: specific operator bytes
    if (b == 0x21 || b == 0x23 || b == 0x25 || b == 0x26 ||
        b == 0x2A || b == 0x2B || b == 0x2D || b == 0x2F ||
        b == 0x3C || b == 0x3D || b == 0x3E ||
        b == 0x5C || b == 0x5E || b == 0x7C) {
        stream_.consume();
        return AstNode::make_list("term2", {AstNode::make_integer(b)});
    }

    throw ParseError("expected term", stream_.position());
}

AstNode Parser::parse_expr() {
    // expr = (expr, many1(term)) -- greedy, no VAL terminator
    std::vector<AstNode> terms;
    terms.push_back(parse_term());

    while (!stream_.at_end() && peek_is_term()) {
        terms.push_back(parse_term());
    }

    return AstNode::make_list("expr", std::move(terms));
}

AstNode Parser::parse_param() {
    uint8_t b = stream_.peek();

    if (b == TOK_BEG) {
        return AstNode::make_list("param", {parse_block()});
    }

    if (b == TOK_STR) {
        return AstNode::make_list("param", {AstNode::make_string(lex_str())});
    }

    return AstNode::make_list("param", {parse_expr()});
}

AstNode Parser::parse_params() {
    // params = optional(param then many(try(CNT param)))
    if (stream_.at_end() || !peek_can_start_param()) {
        return AstNode::make_list("params", {});
    }

    std::vector<AstNode> params;
    params.push_back(parse_param());

    // try to parse more params separated by CNT
    while (!stream_.at_end() && stream_.peek() == TOK_CNT) {
        size_t saved = stream_.position();
        stream_.consume(); // consume CNT

        if (stream_.at_end() || !peek_can_start_param()) {
            stream_.set_position(saved);
            break;
        }

        try {
            params.push_back(parse_param());
        } catch (const ParseError&) {
            stream_.set_position(saved);
            break;
        }
    }

    return AstNode::make_list("params", std::move(params));
}

AstNode Parser::parse_chr_token() {
    return lex_chr();
}

AstNode Parser::parse_chrs() {
    // chrs = (chrs, many1(chr))
    std::vector<AstNode> chars;
    chars.push_back(lex_chr());

    while (!stream_.at_end() && peek_is_chr()) {
        chars.push_back(lex_chr());
    }

    return AstNode::make_list("chrs", std::move(chars));
}

AstNode Parser::parse_cut() {
    stream_.consume(); // consume CNT
    return AstNode::make_cut();
}

AstNode Parser::parse_block() {
    // block = BEG stmts END
    expect(TOK_BEG);
    auto stmts = parse_stmts();
    expect(TOK_END);
    return AstNode::make_list("<>", std::move(stmts));
}

AstNode Parser::parse_block_star() {
    // block* = many(op | chrs) -- can be empty
    std::vector<AstNode> items;

    while (!stream_.at_end()) {

        if (peek_is_op()) {
            items.push_back(parse_op());
        } else if (peek_is_chr()) {
            items.push_back(parse_chrs());
        } else {
            break;
        }
    }

    return AstNode::make_list("<*>", std::move(items));
}

std::vector<AstNode> Parser::parse_cnd_pair() {
    // cnd = CND expr block
    expect(TOK_CND);
    auto expr = parse_expr();
    auto block = parse_block();
    return {std::move(expr), std::move(block)};
}

AstNode Parser::parse_op_str() {
    auto s = lex_str();
    return AstNode::make_list("str", {AstNode::make_string(s)});
}

AstNode Parser::parse_op_cnd() {
    // try op-cnd1 first (more specific pattern with CND expr block)
    auto cnd1_result = try_parse([this]() -> AstNode {
        // first cnd pair
        auto first = parse_cnd_pair();

        // many more cnd pairs separated by CNT
        std::vector<std::vector<AstNode>> more_cnds;

        while (!stream_.at_end() && stream_.peek() == TOK_CNT) {
            auto saved = stream_.position();
            stream_.consume(); // consume CNT

            auto extra_cnd = try_parse([this]() {
                return parse_cnd_pair();
            });

            if (extra_cnd.has_value()) {
                more_cnds.push_back(std::move(*extra_cnd));
            } else {
                stream_.set_position(saved);
                break;
            }
        }

        // optional else clause
        AstNode else_clause = AstNode::make_list("_empty_", {});
        bool has_else = false;

        if (!stream_.at_end() && stream_.peek() == TOK_CNT) {
            auto saved = stream_.position();
            stream_.consume(); // consume CNT

            if (!stream_.at_end() && stream_.peek() == TOK_BEG) {
                // else block (missing CNT before block in dk1/FLOOR4.MES)
                auto block_result = try_parse([this]() {
                    return parse_block();
                });

                if (block_result.has_value()) {
                    else_clause = std::move(*block_result);
                    has_else = true;
                } else {
                    // dangling CNT with missing else
                    else_clause = AstNode::make_list("<*>", {});
                    has_else = true;
                }
            } else {
                // dangling CNT with no block
                else_clause = AstNode::make_list("<*>", {});
                has_else = true;
            }
        } else if (!stream_.at_end() && stream_.peek() == TOK_BEG) {
            // missing CNT before else block (dk1/FLOOR4.MES hack)
            auto block_result = try_parse([this]() {
                return parse_block();
            });

            if (block_result.has_value()) {
                else_clause = std::move(*block_result);
                has_else = true;
            }
        }

        // determine result form
        if (more_cnds.empty() && !has_else) {
            return AstNode::make_list("if", std::move(first));
        }

        if (more_cnds.empty() && has_else) {
            std::vector<AstNode> children = std::move(first);
            children.push_back(std::move(else_clause));
            return AstNode::make_list("if-else", std::move(children));
        }

        if (!more_cnds.empty() && !has_else) {
            std::vector<AstNode> children;
            children.push_back(AstNode::make_list("", std::move(first)));

            for (auto& mc : more_cnds) {
                children.push_back(AstNode::make_list("", std::move(mc)));
            }

            return AstNode::make_list("cond", std::move(children));
        }

        // cond with else
        std::vector<AstNode> children;
        children.push_back(AstNode::make_list("", std::move(first)));

        for (auto& mc : more_cnds) {
            children.push_back(AstNode::make_list("", std::move(mc)));
        }

        children.push_back(AstNode::make_list("else", {std::move(else_clause)}));
        return AstNode::make_list("cond", std::move(children));
    });

    if (cnd1_result.has_value()) {
        return std::move(*cnd1_result);
    }

    // fall back to op-cnd2: CND expr block*
    expect(TOK_CND);
    auto expr = parse_expr();
    auto block = parse_block_star();
    return AstNode::make_list("if", {std::move(expr), std::move(block)});
}

AstNode Parser::parse_op_cmd() {
    // CMD: 0x99-0x9C, 0x9E-0xBF
    uint8_t b = stream_.consume();
    auto cmd = AstNode::make_list("cmd", {AstNode::make_integer(b)});
    auto params = parse_params();

    // flatten: (cmd_children... params)
    std::vector<AstNode> children;

    for (auto& c : cmd.children) {
        children.push_back(std::move(c));
    }

    children.push_back(std::move(params));
    cmd.children = std::move(children);
    return cmd;
}

AstNode Parser::parse_op_proc() {
    return lex_proc();
}

AstNode Parser::parse_op() {
    uint8_t b = stream_.peek();

    // op-str: starts with 0x22 (STR)
    if (b == TOK_STR) {
        return parse_op_str();
    }

    // op-cnd: starts with 0x9D (CND)
    if (b == TOK_CND) {
        return parse_op_cnd();
    }

    // op-cmd: 0x99-0x9C or 0x9E-0xBF
    if ((b >= 0x99 && b <= 0x9C) || (b >= 0x9E && b <= 0xBF)) {
        return parse_op_cmd();
    }

    // op-proc: 0xC0-0xFF
    if (b >= 0xC0) {
        return parse_op_proc();
    }

    throw ParseError("expected operation", stream_.position());
}

AstNode Parser::parse_stmt() {
    uint8_t b = stream_.peek();

    // block: starts with BEG
    if (b == TOK_BEG) {
        return parse_block();
    }

    // cut: starts with CNT
    if (b == TOK_CNT) {
        return parse_cut();
    }

    // op: str, cnd, cmd, proc
    if (peek_is_op()) {
        return parse_op();
    }

    // expr: starts with a term
    if (peek_is_term()) {
        return parse_expr();
    }

    // chrs: starts with chr byte
    if (peek_is_chr()) {
        return parse_chrs();
    }

    throw ParseError("expected statement, got byte " + std::to_string(b), stream_.position());
}

std::vector<AstNode> Parser::parse_stmts() {
    std::vector<AstNode> stmts;

    while (!stream_.at_end() && peek_is_stmt()) {
        stmts.push_back(parse_stmt());
    }

    return stmts;
}

AstNode Parser::parse_mes() {
    // <mes> = stmts END [optional EOF]
    auto stmts = parse_stmts();

    // consume trailing END
    if (!stream_.at_end() && stream_.peek() == TOK_END) {
        stream_.consume();
    }

    // remaining bytes are ignored (garbage before EOF in raygun files)
    return AstNode::make_list("mes", std::move(stmts));
}

// --- utility ---

void Parser::expect(uint8_t b) {
    if (stream_.at_end()) {
        throw ParseError("unexpected end of input, expected " + std::to_string(b), stream_.position());
    }

    uint8_t actual = stream_.consume();

    if (actual != b) {
        throw ParseError(
            "expected byte " + std::to_string(b) + " but got " + std::to_string(actual),
            stream_.position() - 1);
    }
}

template<typename F>
auto Parser::try_parse(F func) -> std::optional<decltype(func())> {
    size_t saved = stream_.position();

    try {
        return func();
    } catch (const ParseError&) {
        stream_.set_position(saved);
        return std::nullopt;
    }
}

bool Parser::peek_is_term() const {
    if (stream_.at_end()) {
        return false;
    }

    uint8_t b = stream_.peek();
    return (b >= 0x01 && b <= 0x07) || // REG0
           b == TOK_REG1 ||             // REG1
           b == TOK_REG2 ||             // REG2
           (b >= 0x11 && b <= 0x17) || // NUM0
           b == TOK_NUM1 ||             // NUM1
           b == TOK_NUM2 ||             // NUM2
           (b >= 0x40 && b <= 0x5A) || // VAR
           b == 0x3F ||                 // TERM0 (?)
           b == 0x21 || b == 0x23 || b == 0x25 || b == 0x26 || // TERM2
           b == 0x2A || b == 0x2B || b == 0x2D || b == 0x2F ||
           b == 0x3C || b == 0x3D || b == 0x3E ||
           b == 0x5C || b == 0x5E || b == 0x7C;
}

bool Parser::peek_is_chr() const {
    if (stream_.at_end()) {
        return false;
    }

    uint8_t b = stream_.peek();
    return b >= 0x80 && b <= 0x98;
}

bool Parser::peek_is_op() const {
    if (stream_.at_end()) {
        return false;
    }

    uint8_t b = stream_.peek();
    return b == TOK_STR ||                                  // op-str
           b == TOK_CND ||                                  // op-cnd
           (b >= 0x99 && b <= 0x9C) ||                     // op-cmd
           (b >= 0x9E && b <= 0xBF) ||                     // op-cmd
           b >= 0xC0;                                       // op-proc
}

bool Parser::peek_is_stmt() const {
    if (stream_.at_end()) {
        return false;
    }

    uint8_t b = stream_.peek();
    return b == TOK_BEG ||     // block
           b == TOK_CNT ||     // cut
           peek_is_op() ||     // op
           peek_is_term() ||   // expr
           peek_is_chr();      // chrs
}

bool Parser::peek_can_start_param() const {
    if (stream_.at_end()) {
        return false;
    }

    uint8_t b = stream_.peek();
    return b == TOK_BEG ||     // block
           b == TOK_STR ||     // STR
           peek_is_term();     // expr (any term)
}

} // namespace ai1
