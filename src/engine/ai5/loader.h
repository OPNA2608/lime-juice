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
