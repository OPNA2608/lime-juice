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

// auto-wrap text nodes in the AST to fit text-frame widths.
// walks the AST tracking the active text-frame width from direct
// (text-frame X1 Y1 X2 Y2) commands and (proc N) calls, then splits
// any (text "...") node whose content exceeds the width into multiple
// nodes with word-boundary padding.
// modifies the AST in place.
void auto_wrap_ast(AstNode& ast);
