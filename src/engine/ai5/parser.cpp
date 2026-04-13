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

namespace ai5 {

Parser::Parser(ByteStream& stream, const Config& cfg)
    : stream_(stream), cfg_(cfg) {}

// --- number decoding ---

int Parser::decode_num(const std::vector<uint8_t>& bytes) {
    // translated from lex-num in mes-parser.rkt
    // recursive: for list (a ... b), result = (decode(a...) << 8 | b) >> 2
    // base case: for single (a), result = a >> 2

    if (bytes.size() == 1) {
        return bytes[0] >> 2;
    }

    // recursive case: split into init and last
    std::vector<uint8_t> init(bytes.begin(), bytes.end() - 1);
    uint8_t last = bytes.back();
    int prefix = decode_num(init);
    return ((prefix << 8) | last) >> 2;
}

// --- lexer functions ---

AstNode Parser::lex_num0() {
    // NUM0: byte 0x30-0x3F → (num, byte - 0x30)
    uint8_t b = stream_.consume();
    return AstNode::make_list("num", {AstNode::make_integer(b - 0x30)});
}

AstNode Parser::lex_num1() {
    // NUM1: 0x07 + 1 byte
    stream_.consume(); // consume 0x07
    std::vector<uint8_t> bytes = {stream_.consume()};
    return AstNode::make_list("num", {AstNode::make_integer(decode_num(bytes))});
}

AstNode Parser::lex_num2() {
    // NUM2: 0x08 + 2 bytes
    stream_.consume(); // consume 0x08
    std::vector<uint8_t> bytes = {stream_.consume(), stream_.consume()};
    return AstNode::make_list("num", {AstNode::make_integer(decode_num(bytes))});
}

AstNode Parser::lex_num3() {
    // NUM3: 0x09 + 3 bytes
    stream_.consume(); // consume 0x09
    std::vector<uint8_t> bytes = {stream_.consume(), stream_.consume(), stream_.consume()};
    return AstNode::make_list("num", {AstNode::make_integer(decode_num(bytes))});
}

AstNode Parser::lex_num() {
    uint8_t b = stream_.peek();

    if (b >= 0x30 && b <= 0x3F) {
        return lex_num0();
    }

    if (b == TOK_NUM1) {
        return lex_num1();
    }

    if (b == TOK_NUM2) {
        return lex_num2();
    }

    if (b == TOK_NUM3) {
        return lex_num3();
    }

    throw ParseError("expected number token", stream_.position());
}

AstNode Parser::lex_var() {
    // VAR: 0x40-0x5A → (var, byte)
    uint8_t b = stream_.consume();
    return AstNode::make_list("var", {AstNode::make_integer(b)});
}

AstNode Parser::lex_term2() {
    // TERM2: 0x20-0x2C → (term2, byte)
    uint8_t b = stream_.consume();
    return AstNode::make_list("term2", {AstNode::make_integer(b)});
}

AstNode Parser::lex_term1() {
    // TERM1: 0x2E → (term1, byte)
    uint8_t b = stream_.consume();
    return AstNode::make_list("term1", {AstNode::make_integer(b)});
}

AstNode Parser::lex_term0() {
    // TERM0: 0x2D or 0x2F → (term0, byte)
    uint8_t b = stream_.consume();
    return AstNode::make_list("term0", {AstNode::make_integer(b)});
}

AstNode Parser::lex_cmd() {
    // CMD: 0x10-0x1F → (cmd, byte)
    uint8_t b = stream_.consume();
    return AstNode::make_list("cmd", {AstNode::make_integer(b)});
}

AstNode Parser::lex_sys() {
    // SYS: 0x04 + opcode [+ extra]
    stream_.consume(); // consume 0x04

    if (cfg_.extra_op) {
        // try esoteric opcodes first (0x29-0x2A + 3 bytes)
        uint8_t b = stream_.peek();

        if (b >= 0x29 && b <= 0x2A) {
            uint8_t opcode = stream_.consume();
            std::vector<uint8_t> extra_bytes = {
                stream_.consume(), stream_.consume(), stream_.consume()
            };
            int extra_val = decode_num(extra_bytes);

            return AstNode::make_list("sys", {
                AstNode::make_integer(opcode),
                AstNode::make_list("num", {AstNode::make_integer(extra_val)})
            });
        }
    }

    // standard opcode: 0x10-0xFF
    uint8_t opcode = stream_.consume();
    return AstNode::make_list("sys", {AstNode::make_integer(opcode)});
}

std::string Parser::lex_str() {
    // STR: 0x06 + printable chars + 0x06
    stream_.consume(); // consume opening 0x06
    std::string result;

    while (!stream_.at_end()) {
        uint8_t b = stream_.peek();

        if (b == TOK_STR) {
            stream_.consume(); // consume closing 0x06
            return result;
        }

        // valid string chars: 0x09 (tab), 0x20-0x7E (printable ASCII), 0xA1-0xDF (half-width katakana)
        if (b == 0x09 || (b >= 0x20 && b <= 0x7E) || (b >= 0xA1 && b <= 0xDF)) {
            result += static_cast<char>(stream_.consume());
        } else {
            throw ParseError("unexpected byte in string: " + std::to_string(b), stream_.position());
        }
    }

    throw ParseError("unterminated string", stream_.position());
}

AstNode Parser::lex_chr() {
    // CHR: SJIS character bytes → (chr-raw, j1+offset, j2)
    uint8_t c1 = stream_.consume();
    uint8_t c2 = stream_.consume();

    // apply offset: +0x20 if using dictionary, 0 otherwise
    int offset = cfg_.use_dict ? 0x20 : 0;
    int j1 = c1 + offset;
    int j2 = c2;

    return AstNode::make_list("chr-raw", {
        AstNode::make_integer(j1),
        AstNode::make_integer(j2)
    });
}

AstNode Parser::lex_dic() {
    // DIC: dictionary reference → (dic, index)
    uint8_t b = stream_.consume();
    int index = b - cfg_.dict_base;
    return AstNode::make_list("dic", {AstNode::make_integer(index)});
}

// --- grammar rules ---

AstNode Parser::parse_term() {
    uint8_t b = stream_.peek();

    // NUM0: 0x30-0x3F
    if (b >= 0x30 && b <= 0x3F) {
        return lex_num0();
    }

    // NUM1: 0x07
    if (b == TOK_NUM1) {
        return lex_num1();
    }

    // NUM2: 0x08
    if (b == TOK_NUM2) {
        return lex_num2();
    }

    // NUM3: 0x09
    if (b == TOK_NUM3) {
        return lex_num3();
    }

    // VAR: 0x40-0x5A
    if (b >= 0x40 && b <= 0x5A) {
        return lex_var();
    }

    // TERM0: 0x2D or 0x2F
    if (b == 0x2D || b == 0x2F) {
        return lex_term0();
    }

    // TERM1: 0x2E
    if (b == 0x2E) {
        return lex_term1();
    }

    // TERM2: 0x20-0x2C
    if (b >= 0x20 && b <= 0x2C) {
        return lex_term2();
    }

    throw ParseError("expected term", stream_.position());
}

AstNode Parser::parse_expr() {
    // expr = (expr, manyTill(term, VAL))
    // collect terms until we hit 0x03 (VAL)
    std::vector<AstNode> terms;

    while (!stream_.at_end() && stream_.peek() != TOK_VAL) {
        terms.push_back(parse_term());
    }

    // consume VAL terminator
    if (!stream_.at_end()) {
        expect(TOK_VAL);
    }

    return AstNode::make_list("expr", std::move(terms));
}

AstNode Parser::parse_exprs() {
    // exprs = (exprs, sepBy1(expr, CNT))
    std::vector<AstNode> exprs;
    exprs.push_back(parse_expr());

    while (!stream_.at_end() && stream_.peek() == TOK_CNT) {
        stream_.consume(); // consume CNT separator
        exprs.push_back(parse_expr());
    }

    return AstNode::make_list("exprs", std::move(exprs));
}

AstNode Parser::parse_param() {
    // param = (param, block | STR | expr)
    uint8_t b = stream_.peek();

    if (b == TOK_BEG) {
        // block
        return AstNode::make_list("param", {parse_block()});
    }

    if (b == TOK_STR) {
        // string
        return AstNode::make_list("param", {AstNode::make_string(lex_str())});
    }

    // expression
    return AstNode::make_list("param", {parse_expr()});
}

AstNode Parser::parse_params() {
    // params = (params, optional(param then many(try(CNT param))))
    // the params production is optional - it can be empty

    // check if there's anything that could start a param
    if (stream_.at_end()) {
        return AstNode::make_list("params", {});
    }

    uint8_t b = stream_.peek();

    // can a param start here? params start with: BEG (block), STR (0x06), or anything that starts an expr (terms)
    bool can_start_param =
        b == TOK_BEG ||
        b == TOK_STR ||
        (b >= 0x30 && b <= 0x3F) || // NUM0
        b == TOK_NUM1 ||             // NUM1
        b == TOK_NUM2 ||             // NUM2
        b == TOK_NUM3 ||             // NUM3
        (b >= 0x40 && b <= 0x5A) || // VAR
        b == 0x2D || b == 0x2F ||   // TERM0
        b == 0x2E ||                 // TERM1
        (b >= 0x20 && b <= 0x2C);   // TERM2

    if (!can_start_param) {
        return AstNode::make_list("params", {});
    }

    std::vector<AstNode> params;
    params.push_back(parse_param());

    // try to parse more params separated by CNT
    while (!stream_.at_end() && stream_.peek() == TOK_CNT) {
        size_t saved = stream_.position();
        stream_.consume(); // consume CNT

        // check if the next thing is actually a param (not cut or something else)
        if (stream_.at_end()) {
            stream_.set_position(saved);
            break;
        }

        uint8_t next = stream_.peek();
        bool is_param =
            next == TOK_BEG ||
            next == TOK_STR ||
            (next >= 0x30 && next <= 0x3F) ||
            next == TOK_NUM1 || next == TOK_NUM2 || next == TOK_NUM3 ||
            (next >= 0x40 && next <= 0x5A) ||
            next == 0x2D || next == 0x2F ||
            next == 0x2E ||
            (next >= 0x20 && next <= 0x2C);

        if (!is_param) {
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

AstNode Parser::parse_chr() {
    uint8_t b = stream_.peek();

    // dictionary reference
    if (cfg_.use_dict && b >= cfg_.dict_base) {
        return lex_dic();
    }

    // SJIS character
    if (cfg_.use_dict) {
        // with dict: chr range is 0x60 to (dictbase-1)
        if (b >= 0x60 && b < cfg_.dict_base) {
            return lex_chr();
        }
    } else {
        // without dict: chr range is 0x80 to 0xFF
        if (b >= 0x80) {
            return lex_chr();
        }
    }

    throw ParseError("expected chr or dic token", stream_.position());
}

AstNode Parser::parse_chrs() {
    // chrs = (chrs, many1 chr)
    std::vector<AstNode> chars;
    chars.push_back(parse_chr());

    while (!stream_.at_end() && peek_is_chr_or_dic()) {
        chars.push_back(parse_chr());
    }

    return AstNode::make_list("chrs", std::move(chars));
}

AstNode Parser::parse_block() {
    // block = (<>, BEG stmts END)
    expect(TOK_BEG);
    auto stmts = parse_stmts();
    expect(TOK_END);
    return AstNode::make_list("<>", std::move(stmts));
}

AstNode Parser::parse_block_star() {
    // block* = (<*>, many(op | chrs))
    std::vector<AstNode> items;

    while (!stream_.at_end()) {

        if (peek_is_op()) {
            items.push_back(parse_op());
        } else if (peek_is_chr_or_dic()) {
            items.push_back(parse_chrs());
        } else {
            break;
        }
    }

    return AstNode::make_list("<*>", std::move(items));
}

std::vector<AstNode> Parser::parse_cnd_pair() {
    // cnd = CND expr block → [expr, block]
    expect(TOK_CND);
    auto expr = parse_expr();
    auto block = parse_block();
    return {std::move(expr), std::move(block)};
}

AstNode Parser::parse_op_cnd() {
    // try op-cnd1 first (the more specific pattern)
    // op-cnd1: try(cnd) then many(try(CNT cnd)) then optional(CNT (block | (<*>)))

    auto cnd1_result = try_parse([this]() -> AstNode {
        // first cnd pair (must succeed via try)
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

        // optional else clause: CNT (block | (<*>))
        AstNode else_clause = AstNode::make_list("_empty_", {});
        bool has_else = false;

        if (!stream_.at_end() && stream_.peek() == TOK_CNT) {
            auto saved = stream_.position();
            stream_.consume(); // consume CNT

            if (!stream_.at_end() && stream_.peek() == TOK_BEG) {
                // else block
                auto block_result = try_parse([this]() {
                    return parse_block();
                });

                if (block_result.has_value()) {
                    else_clause = std::move(*block_result);
                    has_else = true;
                } else {
                    // dangling CNT with missing else → (<*>)
                    else_clause = AstNode::make_list("<*>", {});
                    has_else = true;
                }
            } else {
                // dangling CNT with no block → (<*>)
                else_clause = AstNode::make_list("<*>", {});
                has_else = true;
            }
        }

        // determine result form based on what we parsed
        if (more_cnds.empty() && !has_else) {
            // simple if: (if expr block)
            return AstNode::make_list("if", std::move(first));
        }

        if (more_cnds.empty() && has_else) {
            // if-else: (if-else expr block else-block)
            std::vector<AstNode> children = std::move(first);
            children.push_back(std::move(else_clause));
            return AstNode::make_list("if-else", std::move(children));
        }

        if (!more_cnds.empty() && !has_else) {
            // cond: (cond (expr block) (expr block) ...)
            std::vector<AstNode> children;
            children.push_back(AstNode::make_list("", std::move(first)));

            for (auto& mc : more_cnds) {
                children.push_back(AstNode::make_list("", std::move(mc)));
            }

            return AstNode::make_list("cond", std::move(children));
        }

        // cond with else: (cond (expr block) ... (else else-block))
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

AstNode Parser::parse_op_sys() {
    // op-sys = SYS params → flatten sys + params children
    auto sys = lex_sys();
    auto params = parse_params();

    // result is a flat list: (sys_children... params_children...)
    // in racket: (:: SYS params) produces a list of both results
    std::vector<AstNode> children;

    for (auto& c : sys.children) {
        children.push_back(std::move(c));
    }

    children.push_back(std::move(params));
    sys.children = std::move(children);
    return sys;
}

AstNode Parser::parse_op_str() {
    // op-str = (str, STR)
    auto s = lex_str();
    return AstNode::make_list("str", {AstNode::make_string(s)});
}

AstNode Parser::parse_op_set() {
    uint8_t b = stream_.peek();

    if (b == TOK_SETRC) {
        // set-reg: = SETRC NUM exprs
        stream_.consume();
        auto num = lex_num();
        auto exprs = parse_exprs();
        return AstNode::make_list("set-reg:", {std::move(num), std::move(exprs)});
    }

    if (b == TOK_SETRE) {
        // set-reg:: = SETRE expr exprs
        stream_.consume();
        auto expr = parse_expr();
        auto exprs = parse_exprs();
        return AstNode::make_list("set-reg::", {std::move(expr), std::move(exprs)});
    }

    if (b == TOK_SETV) {
        // set-var = SETV VAR expr
        stream_.consume();
        auto var = lex_var();
        auto expr = parse_expr();
        return AstNode::make_list("set-var", {std::move(var), std::move(expr)});
    }

    if (b == TOK_SETAW) {
        // set-arr~ = SETAW VAR expr exprs
        stream_.consume();
        auto var = lex_var();
        auto expr = parse_expr();
        auto exprs = parse_exprs();
        return AstNode::make_list("set-arr~", {std::move(var), std::move(expr), std::move(exprs)});
    }

    if (b == TOK_SETAB) {
        // set-arr~b = SETAB VAR expr exprs
        stream_.consume();
        auto var = lex_var();
        auto expr = parse_expr();
        auto exprs = parse_exprs();
        return AstNode::make_list("set-arr~b", {std::move(var), std::move(expr), std::move(exprs)});
    }

    throw ParseError("expected set operation", stream_.position());
}

AstNode Parser::parse_op_cmd() {
    // op-cmd = CMD params
    auto cmd = lex_cmd();
    auto params = parse_params();

    // result: flat list (cmd_children... params)
    std::vector<AstNode> children;

    for (auto& c : cmd.children) {
        children.push_back(std::move(c));
    }

    children.push_back(std::move(params));
    cmd.children = std::move(children);
    return cmd;
}

AstNode Parser::parse_op() {
    uint8_t b = stream_.peek();

    // op-sys: starts with 0x04
    if (b == TOK_SYS) {
        return parse_op_sys();
    }

    // op-str: starts with 0x06
    if (b == TOK_STR) {
        return parse_op_str();
    }

    // op-set: starts with 0x0A-0x0E
    if (b >= TOK_SETRC && b <= TOK_SETAB) {
        return parse_op_set();
    }

    // op-cnd: starts with 0x0F
    if (b == TOK_CND) {
        return parse_op_cnd();
    }

    // op-cmd: starts with 0x10-0x1F
    if (b >= 0x10 && b <= 0x1F) {
        return parse_op_cmd();
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
        stream_.consume();
        return AstNode::make_cut();
    }

    // op: starts with SYS, STR, SET*, CND, CMD
    if (peek_is_op()) {
        return parse_op();
    }

    // chrs: starts with chr or dic byte
    if (peek_is_chr_or_dic()) {
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
    // <mes> = (mes, stmts optional(END) optional(eof))
    auto stmts = parse_stmts();

    // optional trailing END
    if (!stream_.at_end() && stream_.peek() == TOK_END) {
        stream_.consume();
    }

    // remaining bytes are ignored (inconsistent endings in some files)
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

bool Parser::peek_is(uint8_t b) const {
    return !stream_.at_end() && stream_.peek() == b;
}

bool Parser::peek_in_range(uint8_t lo, uint8_t hi) const {
    if (stream_.at_end()) {
        return false;
    }

    uint8_t b = stream_.peek();
    return b >= lo && b <= hi;
}

bool Parser::peek_is_chr_or_dic() const {
    if (stream_.at_end()) {
        return false;
    }

    uint8_t b = stream_.peek();

    if (cfg_.use_dict) {
        // chr: 0x60 to (dictbase-1), dic: dictbase to 0xFF
        return b >= 0x60;
    } else {
        // chr: 0x80 to 0xFF (no dic)
        return b >= 0x80;
    }
}

bool Parser::peek_is_op() const {
    if (stream_.at_end()) {
        return false;
    }

    uint8_t b = stream_.peek();
    return b == TOK_SYS ||                           // op-sys
           b == TOK_STR ||                           // op-str
           (b >= TOK_SETRC && b <= TOK_SETAB) ||     // op-set
           b == TOK_CND ||                           // op-cnd
           (b >= 0x10 && b <= 0x1F);                 // op-cmd
}

bool Parser::peek_is_stmt() const {
    if (stream_.at_end()) {
        return false;
    }

    uint8_t b = stream_.peek();
    return b == TOK_BEG ||       // block
           b == TOK_CNT ||       // cut
           peek_is_op() ||       // op
           peek_is_chr_or_dic(); // chrs
}

} // namespace ai5
