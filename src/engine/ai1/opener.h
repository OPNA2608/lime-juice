#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ai1 {

// read a .MES file as raw bytecode (no header, no dictionary)
// translated from engine/ai1/mes-opener.rkt
std::vector<uint8_t> open_mes(const std::string& path);

} // namespace ai1
