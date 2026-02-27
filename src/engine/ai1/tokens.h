#pragma once

#include <cstdint>

namespace ai1 {

static constexpr uint8_t TOK_REG1 = 0x00;
static constexpr uint8_t TOK_REG0 = 0x01;
static constexpr uint8_t TOK_REG2 = 0x08;
static constexpr uint8_t TOK_NUM1 = 0x10;
static constexpr uint8_t TOK_NUM0 = 0x11;
static constexpr uint8_t TOK_NUM2 = 0x18;
static constexpr uint8_t TOK_STR  = 0x22;
static constexpr uint8_t TOK_CNT  = 0x2C;
static constexpr uint8_t TOK_BEG  = 0x7B;
static constexpr uint8_t TOK_END  = 0x7D;
static constexpr uint8_t TOK_CND  = 0x9D;
static constexpr uint8_t TOK_PROC = 0xC0;

} // namespace ai1
