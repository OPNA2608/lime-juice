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

#include "opener.h"

#include <fstream>
#include <stdexcept>

namespace ai5 {

MesFile open_mes(const std::string& path) {
    // read entire file into memory
    std::ifstream file(path, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("cannot open file: " + path);
    }

    std::vector<uint8_t> bytes(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    return open_mes_bytes(bytes);
}

MesFile open_mes_bytes(const std::vector<uint8_t>& bytes) {
    // translated from mes-opener.rkt open-mes-bytes
    // format:
    //   [offset: 2 bytes, little-endian] - marks end of dictionary, start of code
    //   [dictionary: (offset - 2) bytes] - pairs of SJIS character bytes
    //   [code: remaining bytes] - bytecode stream

    if (bytes.size() < 2) {
        throw std::runtime_error("MES file too small (< 2 bytes)");
    }

    // read 2-byte little-endian offset
    uint16_t offset = bytes[0] | (bytes[1] << 8);

    if (offset > bytes.size()) {
        throw std::runtime_error("MES header offset exceeds file size (try AI1 or ADV engine)");
    }

    // extract dictionary (bytes 2 through offset-1, as pairs)
    MesFile mes;
    size_t dict_start = 2;
    size_t dict_end = offset;

    for (size_t i = dict_start; i + 1 < dict_end; i += 2) {
        mes.dictionary.push_back({bytes[i], bytes[i + 1]});
    }

    // extract code (bytes from offset to end)
    mes.code.assign(bytes.begin() + offset, bytes.end());

    return mes;
}

} // namespace ai5
