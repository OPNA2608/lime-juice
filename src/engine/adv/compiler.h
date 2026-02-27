#pragma once

#include "../../ast.h"
#include "../../config.h"

#include <cstdint>
#include <string>
#include <vector>

namespace adv {

// compile a fused AST back into MES bytecode
// the input AST is the same format that load_mes produces (and SexpWriter formats)
std::vector<uint8_t> compile_mes(const AstNode& ast, const Config& cfg);

} // namespace adv
