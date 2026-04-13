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
