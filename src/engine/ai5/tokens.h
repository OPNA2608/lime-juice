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

namespace ai5 {

static constexpr uint8_t TOK_END   = 0x00;
static constexpr uint8_t TOK_BEG   = 0x01;
static constexpr uint8_t TOK_CNT   = 0x02;
static constexpr uint8_t TOK_VAL   = 0x03;
static constexpr uint8_t TOK_SYS   = 0x04;
static constexpr uint8_t TOK_STR   = 0x06;
static constexpr uint8_t TOK_NUM1  = 0x07;
static constexpr uint8_t TOK_NUM2  = 0x08;
static constexpr uint8_t TOK_NUM3  = 0x09;
static constexpr uint8_t TOK_SETRC = 0x0A;
static constexpr uint8_t TOK_SETRE = 0x0B;
static constexpr uint8_t TOK_SETV  = 0x0C;
static constexpr uint8_t TOK_SETAW = 0x0D;
static constexpr uint8_t TOK_SETAB = 0x0E;
static constexpr uint8_t TOK_CND   = 0x0F;

} // namespace ai5
