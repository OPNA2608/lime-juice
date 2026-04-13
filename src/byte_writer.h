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

#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

// simple byte output buffer for compiling AST back to MES bytecode

class ByteWriter {
public:
    void emit(uint8_t byte) {
        buf_.push_back(byte);
    }

    void emit(const std::vector<uint8_t>& bytes) {
        buf_.insert(buf_.end(), bytes.begin(), bytes.end());
    }

    void emit_u16_le(uint16_t val) {
        buf_.push_back(static_cast<uint8_t>(val & 0xFF));
        buf_.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    }

    void emit_u16_be(uint16_t val) {
        buf_.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
        buf_.push_back(static_cast<uint8_t>(val & 0xFF));
    }

    const std::vector<uint8_t>& data() const { return buf_; }
    std::vector<uint8_t> take_data() { return std::move(buf_); }
    size_t size() const { return buf_.size(); }

    // write a value at a specific position (for backpatching)
    void write_u16_le_at(size_t offset, uint16_t val) {

        if (offset + 1 >= buf_.size()) {
            throw std::runtime_error("write_u16_le_at: offset out of range");
        }

        buf_[offset] = static_cast<uint8_t>(val & 0xFF);
        buf_[offset + 1] = static_cast<uint8_t>((val >> 8) & 0xFF);
    }

    void clear() { buf_.clear(); }

private:
    std::vector<uint8_t> buf_;
};
