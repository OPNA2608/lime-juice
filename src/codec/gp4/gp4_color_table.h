#pragma once

#include <array>
#include <cstdint>

namespace gp4 {

// 17-slot rotating MRU table for GP4 color encoding/decoding.
// each slot is a permutation of [0..15]. slot tracks current state (0..16).
class ColorTable {
    std::array<std::array<uint8_t, 16>, 17> rotations_;
    int slot_ = 16;

public:
    ColorTable() {

        for (int s = 0; s < 17; s++) {

            for (int i = 0; i < 16; i++) {
                rotations_[s][i] = (s + i) % 16;
            }
        }
    }

    // find position of color in current rotation
    int peek(uint8_t color) const {
        const auto& rot = rotations_[slot_];

        for (int i = 0; i < 16; i++) {

            if (rot[i] == color) {
                return i;
            }
        }

        return 0;
    }

    // move element at position to front, advance slot
    uint8_t poke(int position) {
        auto& rot = rotations_[slot_];
        uint8_t color = rot[position];

        // shift elements: move position to front
        for (int i = position; i > 0; i--) {
            rot[i] = rot[i - 1];
        }

        rot[0] = color;

        // advance slot to the color value
        slot_ = color;
        return color;
    }

    void reset() {
        slot_ = 16;
    }
};

} // namespace gp4
