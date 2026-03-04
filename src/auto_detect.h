#pragma once

#include "config.h"

#include <cstdint>
#include <vector>

// detect engine type from raw MES file bytes
// checks ADV signature (last 2 bytes FF FE), then AI5 dictionary offset, then AI1 fallback
EngineType detect_engine(const std::vector<uint8_t>& bytes);
