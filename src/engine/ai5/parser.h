#pragma once

#include "../../ast.h"
#include "../../byte_stream.h"
#include "../../config.h"
#include "../engine.h"
#include "tokens.h"

namespace ai5 {

// recursive descent parser for AI5 MES bytecode
// translated from engine/ai5/mes-parser.rkt
class Parser {
public:
    Parser(ByteStream& stream, const Config& cfg);

    // parse the entire MES bytecode stream
    // returns (mes stmt1 stmt2 ...)
    AstNode parse_mes();

private:
    ByteStream& stream_;
    const Config& cfg_;

    // --- number decoding ---

    // decode variable-length number from byte list (lex-num)
    static int decode_num(const std::vector<uint8_t>& bytes);

    // --- lexer functions (token parsers) ---

    // NUM0: single-byte number 0x30-0x3F → (num, value)
    AstNode lex_num0();

    // NUM1: 0x07 + 1 byte → (num, value)
    AstNode lex_num1();

    // NUM2: 0x08 + 2 bytes → (num, value)
    AstNode lex_num2();

    // NUM3: 0x09 + 3 bytes → (num, value)
    AstNode lex_num3();

    // NUM: any number token
    AstNode lex_num();

    // VAR: 0x40-0x5A → (var, byte)
    AstNode lex_var();

    // TERM2: 0x20-0x2C → (term2, byte)
    AstNode lex_term2();

    // TERM1: 0x2E → (term1, byte)
    AstNode lex_term1();

    // TERM0: 0x2D or 0x2F → (term0, byte)
    AstNode lex_term0();

    // CMD: 0x10-0x1F → (cmd, byte)
    AstNode lex_cmd();

    // SYS: 0x04 + opcode [+ extra] → (sys, opcode, [extra_num])
    AstNode lex_sys();

    // STR: 0x06 + printable chars + 0x06 → string
    std::string lex_str();

    // CHR: SJIS character bytes → (chr-raw, j1, j2)
    AstNode lex_chr();

    // DIC: dictionary reference → (dic, index)
    AstNode lex_dic();

    // --- grammar rules ---

    // term = NUM | VAR | TERM0 | TERM1 | TERM2
    AstNode parse_term();

    // expr = (expr, terms until VAL)
    AstNode parse_expr();

    // exprs = (exprs, exprs separated by CNT)
    AstNode parse_exprs();

    // param = (param, block | STR | expr)
    AstNode parse_param();

    // params = (params, params separated by CNT)
    AstNode parse_params();

    // chr = CHR | DIC
    AstNode parse_chr();

    // chrs = (chrs, many1 chr)
    AstNode parse_chrs();

    // block = (<>, BEG stmts END)
    AstNode parse_block();

    // block* = (<*>, many(op | chrs))
    AstNode parse_block_star();

    // cnd = CND expr block (helper for op-cnd)
    // returns pair: (expr, block)
    std::vector<AstNode> parse_cnd_pair();

    // op-cnd = conditional statement (if, if-else, cond)
    AstNode parse_op_cnd();

    // op-sys = SYS params
    AstNode parse_op_sys();

    // op-str = (str, STR)
    AstNode parse_op_str();

    // op-set = set-reg: | set-reg:: | set-var | set-arr~ | set-arr~b
    AstNode parse_op_set();

    // op-cmd = CMD params
    AstNode parse_op_cmd();

    // op = op-sys | op-str | op-set | op-cnd | op-cmd
    AstNode parse_op();

    // stmt = block | cut | op | chrs
    AstNode parse_stmt();

    // stmts = many stmt
    std::vector<AstNode> parse_stmts();

    // --- utility ---

    // consume a specific byte, throw if mismatch
    void expect(uint8_t b);

    // try to parse; on failure, restore position and return nullopt
    template<typename F>
    auto try_parse(F func) -> std::optional<decltype(func())>;

    // check if the next byte matches (without consuming)
    bool peek_is(uint8_t b) const;

    // check if the next byte is in range [lo, hi] (without consuming)
    bool peek_in_range(uint8_t lo, uint8_t hi) const;

    // check if the next byte could start a chr or dic token
    bool peek_is_chr_or_dic() const;

    // check if the next byte could start an op
    bool peek_is_op() const;

    // check if the next byte could start a stmt
    bool peek_is_stmt() const;
};

} // namespace ai5
