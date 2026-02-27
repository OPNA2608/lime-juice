#pragma once

#include "../../ast.h"
#include "../../charset.h"
#include "../../config.h"

#include <string>
#include <vector>

namespace adv {

struct LoadResult {
    AstNode ast;
    std::vector<std::string> warnings;
};

// full decompile pipeline: open → parse → lower → resolve → fuse
// translated from engine/adv/mes-loader.rkt
LoadResult load_mes(const std::string& path, Config& cfg);

// --- pipeline stages (exposed for testing) ---

// lower: convert raw parse tree to symbolic form
AstNode lower(const AstNode& node, const Config& cfg, const Charset& cs);

// resolve: map opcode bytes to human-readable names
AstNode resolve(const AstNode& node, const Config& cfg);

// fuse: 9 sequential tree transformations
AstNode fuse(const AstNode& node, const Config& cfg, const Charset& cs);

} // namespace adv
