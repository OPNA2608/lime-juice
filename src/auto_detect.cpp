#include "auto_detect.h"

#include <algorithm>

// score a byte sequence for AI5 vs AI1 structural markers.
// scans the given range and returns positive if AI5 markers dominate,
// negative if AI1 markers dominate.
// works entirely heuristically (vibes), so false positives are possible
// but should be rare.
static int score_engine_markers(const std::vector<uint8_t>& bytes,
                                size_t start, size_t end) {
    int ai5 = 0;
    int ai1 = 0;

    for (size_t i = start; i < end; i++) {
        uint8_t b = bytes[i];

        // AI5: 0x04 (SYS) is always followed by a system function number.
        // this 2-byte pattern is highly distinctive to AI5.

        if (b == 0x04 && i + 1 < end) {
            ai5 += 5;
            i++;
            continue;
        }

        // AI5: 0x03 (VAL) terminates expressions, appears very frequently

        if (b == 0x03) {
            ai5 += 2;
            continue;
        }

        // AI5: 0x06 (STR) string delimiter

        if (b == 0x06) {
            ai5 += 3;
            continue;
        }

        // AI5: 0x0F (CND) conditional

        if (b == 0x0F) {
            ai5 += 2;
            continue;
        }

        // AI1: 0x7B (BEG) block begin

        if (b == 0x7B) {
            ai1 += 3;
            continue;
        }

        // AI1: 0x7D (END) block end

        if (b == 0x7D) {
            ai1 += 3;
            continue;
        }

        // AI1: 0x9D (CND) conditional, very distinctive

        if (b == 0x9D) {
            ai1 += 4;
            continue;
        }

        // AI1: 0x22 (STR) string delimiter

        if (b == 0x22) {
            ai1 += 2;
            continue;
        }
    }

    return ai5 - ai1;
}

// detect engine type from raw MES file bytes
EngineType detect_engine(const std::vector<uint8_t>& bytes) {

    if (bytes.size() < 2) {
        return EngineType::AI1;
    }

    // check ADV: last 2 bytes are FF FE (end-of-mes marker)

    if (bytes[bytes.size() - 2] == 0xFF && bytes[bytes.size() - 1] == 0xFE) {
        return EngineType::ADV;
    }

    // check AI5: first 2 bytes form a valid dictionary offset (LE uint16).
    // offset == 2 means empty dictionary, which is valid.
    // (offset - 2) must be even since the dictionary consists of byte pairs.
    uint16_t offset = static_cast<uint16_t>(bytes[0] | (bytes[1] << 8));

    if (offset >= 2 && offset <= bytes.size() && (offset - 2) % 2 == 0) {

        // the offset check alone produces false positives for AI1 files whose
        // first 2 bytes coincidentally form a valid-looking offset.
        // validate by scoring engine-specific structural byte patterns.
        // scan both the supposed code section AND from byte 0, since AI1 files
        // that produce large offset values may have text data (not code) at
        // the offset position, but will always have structural markers near
        // the start of the file.
        int score = 0;

        // scan the supposed code section (bytes from offset onward)
        size_t code_start = static_cast<size_t>(offset);
        size_t code_scan_end = std::min(bytes.size(), code_start + 512);

        if (code_start < bytes.size()) {
            score += score_engine_markers(bytes, code_start, code_scan_end);
        }

        // also scan from the beginning of the file.
        // for AI1, this is raw bytecode with structural markers (0x7B, 0x7D).
        // for AI5, this is the offset header + dictionary (high bytes, neutral score).
        size_t head_scan_end = std::min(bytes.size(), static_cast<size_t>(512));
        score += score_engine_markers(bytes, 0, head_scan_end);

        if (score < 0) {
            return EngineType::AI1;
        }

        return EngineType::AI5;
    }

    // fallback
    return EngineType::AI1;
}
