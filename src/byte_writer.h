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
