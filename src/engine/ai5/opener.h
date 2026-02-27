#pragma once

#include "../engine.h"
#include <string>
#include <vector>

namespace ai5 {

// parse a .MES binary file into dictionary + code
// translated from engine/ai5/mes-opener.rkt
MesFile open_mes(const std::string& path);

// parse raw bytes into MesFile
MesFile open_mes_bytes(const std::vector<uint8_t>& bytes);

} // namespace ai5
