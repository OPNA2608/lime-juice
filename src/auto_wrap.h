#pragma once

#include "ast.h"

// auto-wrap text nodes in the AST to fit text-frame widths.
// walks the AST tracking the active text-frame width from direct
// (text-frame X1 Y1 X2 Y2) commands and (proc N) calls, then splits
// any (text "...") node whose content exceeds the width into multiple
// nodes with word-boundary padding.
// modifies the AST in place.
void auto_wrap_ast(AstNode& ast);
