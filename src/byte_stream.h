#pragma once

#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

// byte buffer with position tracking for the recursive descent parser
class ByteStream {
public:
    explicit ByteStream(std::vector<uint8_t> data)
        : data_(std::move(data)), pos_(0) {}

    // construct from a raw string (for compatibility with latin-1 encoded code sections)
    explicit ByteStream(const std::string& s)
        : data_(s.begin(), s.end()), pos_(0) {}

    uint8_t peek() const {
        if (pos_ >= data_.size()) {
            throw std::runtime_error("ByteStream: unexpected end of input at position " + std::to_string(pos_));
        }
        return data_[pos_];
    }

    uint8_t consume() {
        uint8_t b = peek();
        pos_++;
        return b;
    }

    bool at_end() const {
        return pos_ >= data_.size();
    }

    size_t position() const {
        return pos_;
    }

    void set_position(size_t pos) {
        pos_ = pos;
    }

    size_t size() const {
        return data_.size();
    }

    const std::vector<uint8_t>& data() const {
        return data_;
    }

private:
    std::vector<uint8_t> data_;
    size_t pos_;
};
