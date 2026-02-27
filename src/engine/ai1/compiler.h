#pragma once

#include "../../ast.h"
#include "../../byte_writer.h"
#include "../../config.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ai1 {

// compile a fused AST back into MES bytecode
// the input AST is the same format that load_mes produces (and SexpWriter formats)
std::vector<uint8_t> compile_mes(const AstNode& ast, Config& cfg);

// shared utility: emit a UTF-8 string as raw bytes, converting
// half-width katakana to JIS X 0201 and collapsing 2-byte sequences
void emit_str_bytes(ByteWriter& out, const std::string& s);

} // namespace ai1
