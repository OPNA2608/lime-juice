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

#include "../../ast.h"
#include "../../charset.h"
#include "../../config.h"
#include "../engine.h"

#include <string>

namespace ai5 {

// full decompile pipeline: open → parse → lower → resolve → fuse
// translated from engine/ai5/mes-loader.rkt
AstNode load_mes(const std::string& path, Config& cfg);

// --- pipeline stages (exposed for testing) ---

// lower: convert raw parse tree to symbolic form
// - unwrap (num n) to plain integers
// - convert (var byte) to variable symbols
// - decode (chr-raw j1 j2) to characters
// - fold expressions to prefix notation
AstNode lower(const AstNode& node, const Config& cfg, const Charset& cs);

// resolve: map opcode bytes to human-readable names
// - (exp byte) → operator symbol (+, -, *, etc.)
// - (cmd byte) → command name (wait, proc, etc.)
// - (sys byte [extra]) → system function name (while, load, etc.)
AstNode resolve(const AstNode& node, const Config& cfg);

// fuse: 10 sequential tree transformations
AstNode fuse(const AstNode& node, const std::vector<std::vector<int>>& dict, const Config& cfg, const Charset& cs);

} // namespace ai5
