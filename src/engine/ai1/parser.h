#pragma once

#include "../../ast.h"
#include "../../byte_stream.h"
#include "../../config.h"
#include "../engine.h"
#include "tokens.h"

namespace ai1 {

// recursive descent parser for AI1 MES bytecode
// translated from engine/ai1/mes-parser.rkt
class Parser {
public:
    Parser(ByteStream& stream, const Config& cfg);

    // parse the entire MES bytecode stream
    // returns (mes stmt1 stmt2 ...)
    AstNode parse_mes();

private:
    ByteStream& stream_;
    const Config& cfg_;

    // --- token byte ranges ---
    // AI1 uses ASCII-like token values (different from AI5)
    //   REG0: 0x01-0x07, REG1: 0x00+byte, REG2: 0x08+2bytes
    //   NUM0: 0x11-0x17, NUM1: 0x10+byte, NUM2: 0x18+2bytes
    //   STR:  0x22...0x22
    //   VAR:  0x40-0x5A
    //   BEG:  0x7B '{', END: 0x7D '}', CNT: 0x2C ','
    //   CHR:  0x80-0x98 + 0x40-0xFC (SJIS pair)
    //   CND:  0x9D
    //   CMD:  0x99-0x9C, 0x9E-0xBF
    //   PROC: 0xC0-0xFF

    // --- lexer functions ---

    // REG0: 0x01-0x07, value = byte - 1
    AstNode lex_reg0();

    // REG1: 0x00 + 1 byte
    AstNode lex_reg1();

    // REG2: 0x08 + 2 bytes
    AstNode lex_reg2();

    // NUM0: 0x11-0x17, value = byte - 0x11
    AstNode lex_num0();

    // NUM1: 0x10 + 1 byte
    AstNode lex_num1();

    // NUM2: 0x18 + 2 bytes
    AstNode lex_num2();

    // STR: 0x22 + chars + 0x22
    std::string lex_str();

    // CHR: SJIS character pair (0x80-0x98 + 0x40-0xFC)
    AstNode lex_chr();

    // PROC: 0xC0-0xFF, value = byte - 0xC0
    AstNode lex_proc();

    // --- grammar rules ---

    // REG = REG0 | REG1 | REG2
    AstNode parse_reg();

    // NUM = NUM0 | NUM1 | NUM2
    AstNode parse_num();

    // term = REG | NUM | VAR | TERM0 | TERM2
    AstNode parse_term();

    // expr = (expr, many1(term))  -- greedy, no VAL terminator
    AstNode parse_expr();

    // param = block | STR | expr
    AstNode parse_param();

    // params = optional(param then many(try(CNT param)))
    AstNode parse_params();

    // chr (single)
    AstNode parse_chr_token();

    // chrs = (chrs, many1(chr))
    AstNode parse_chrs();

    // cut = CNT → (cut)
    AstNode parse_cut();

    // block = BEG stmts END
    AstNode parse_block();

    // block* = many(op | chrs) -- can be empty
    AstNode parse_block_star();

    // cnd = CND expr block (helper for conditionals)
    std::vector<AstNode> parse_cnd_pair();

    // op-str = (str, STR)
    AstNode parse_op_str();

    // op-cnd = conditional (if/if-else/cond)
    AstNode parse_op_cnd();

    // op-cmd = CMD params
    AstNode parse_op_cmd();

    // op-proc = PROC
    AstNode parse_op_proc();

    // op = op-str | op-cnd | op-cmd | op-proc
    AstNode parse_op();

    // stmt = block | cut | op | expr | chrs
    AstNode parse_stmt();

    // stmts = many(stmt)
    std::vector<AstNode> parse_stmts();

    // --- utility ---

    void expect(uint8_t b);

    template<typename F>
    auto try_parse(F func) -> std::optional<decltype(func())>;

    bool peek_is_term() const;
    bool peek_is_chr() const;
    bool peek_is_op() const;
    bool peek_is_stmt() const;
    bool peek_can_start_param() const;
};

} // namespace ai1
