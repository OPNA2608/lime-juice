#include "opener.h"

#include <fstream>
#include <stdexcept>

namespace adv {

std::vector<uint8_t> open_mes(const std::string& path) {
    // adv files are raw bytecode with no header or dictionary
    std::ifstream file(path, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("cannot open file: " + path);
    }

    std::vector<uint8_t> bytes(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    return bytes;
}

} // namespace adv
