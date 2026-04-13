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

#include <string>

namespace ai1 {

// full decompile pipeline: open -> parse -> lower -> resolve -> fuse
// translated from engine/ai1/mes-loader.rkt
AstNode load_mes(const std::string& path, Config& cfg);

// --- pipeline stages (exposed for testing) ---

// lower: convert raw parse tree to symbolic form
AstNode lower(const AstNode& node, const Config& cfg, const Charset& cs);

// resolve: map opcode bytes to human-readable names
AstNode resolve(const AstNode& node, const Config& cfg);

// fuse: 9 sequential tree transformations
AstNode fuse(const AstNode& node, const Config& cfg);

// --- shared fuse utilities (also used by AI5) ---

AstNode fold_expr(const std::vector<AstNode>& terms);
void flatten_assoc_op(const AstNode& node, const std::string& op, std::vector<AstNode>& out);
AstNode fuse_operator(const AstNode& node);
bool is_same_line(const AstNode& text_item);
AstNode fuse_while(const AstNode& node);
AstNode fuse_text(const AstNode& node);
AstNode fuse_text_proc_call(const AstNode& node, const Config& cfg);
AstNode fuse_text_number(const AstNode& node);
AstNode fuse_text_multiple(const AstNode& node);
AstNode fuse_text_color(const AstNode& node);
AstNode fuse_menu_block(const AstNode& node);

} // namespace ai1
