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

#include "loader.h"
#include "opener.h"
#include "parser.h"
#include "../../charset.h"
#include "../../utf8.h"

#include <stdexcept>

namespace adv {

// forward declarations
static AstNode lower_impl(const AstNode& node, const Config& cfg, const Charset& cs);
static AstNode resolve_impl(const AstNode& node, const Config& cfg);

// fuse passes
static AstNode fuse_arg(const AstNode& node, const Charset& cs);
static AstNode fuse_str(const AstNode& node, const Charset& cs);
static AstNode fuse_text(const AstNode& node);
static AstNode fuse_text_break(const AstNode& node);
static AstNode fuse_text_proc(const AstNode& node, const Config& cfg);
static AstNode fuse_text_raw(const AstNode& node);
static AstNode fuse_text_pos(const AstNode& node);
static AstNode fuse_text_color(const AstNode& node);
static AstNode fuse_meta(const AstNode& node, const Config& cfg);

// --- main pipeline ---

LoadResult load_mes(const std::string& path, Config& cfg) {
    Charset cs;
    cs.load(cfg.charset_name);

    cfg.char_space = cs.space_char();
    cfg.char_newline = cs.newline_char();

    auto bytes = open_mes(path);
    ByteStream stream(std::move(bytes));
    Parser parser(stream, cfg);
    auto parsed = parser.parse_mes();

    auto lowered = lower(parsed, cfg, cs);
    auto resolved = resolve(lowered, cfg);
    auto fused = fuse(resolved, cfg, cs);

    return LoadResult{std::move(fused), parser.warnings()};
}

// --- lower ---

AstNode lower(const AstNode& node, const Config& cfg, const Charset& cs) {
    return lower_impl(node, cfg, cs);
}

static AstNode lower_var(char32_t c) {
    // convert character to symbol name
    return AstNode::make_symbol(char32_to_utf8(c));
}

static AstNode lower_chr_ascii(char32_t c) {
    // handle modified ASCII characters: \ → ¥, ~ → ‾
    if (c == U'\\') {
        return AstNode::make_list("chr", {AstNode::make_character(U'\u00A5')}); // ¥
    }

    if (c == U'~') {
        return AstNode::make_list("chr", {AstNode::make_character(U'\u203E')}); // ‾
    }

    return AstNode::make_list("chr", {AstNode::make_character(c)});
}

static AstNode lower_chr_sjis1(int i, const Charset& cs) {
    auto opt = cs.sjis_to_char({i, 0});

    if (opt.has_value()) {
        return AstNode::make_list("chr", {AstNode::make_character(*opt)});
    }

    return AstNode::make_list("chr-raw", {AstNode::make_integer(i), AstNode::make_integer(0)});
}

static AstNode lower_chr_sjis2(const std::vector<int>& j, const Config& cfg, const Charset& cs) {
    auto opt = cs.sjis_to_char(j);

    if (opt.has_value() && cfg.decode) {
        return AstNode::make_list("chr", {AstNode::make_character(*opt)});
    }

    std::vector<AstNode> children;

    for (int b : j) {
        children.push_back(AstNode::make_integer(b));
    }

    return AstNode::make_list("chr-raw", std::move(children));
}

static AstNode lower_chr_sjis2_plus(const std::vector<int>& j) {
    // always raw
    std::vector<AstNode> children;

    for (int b : j) {
        children.push_back(AstNode::make_integer(b));
    }

    return AstNode::make_list("chr-raw", std::move(children));
}

static AstNode lower_impl(const AstNode& node, const Config& cfg, const Charset& cs) {
    // translated from lower in adv/mes-loader.rkt

    if (node.is_list("num")) {
        // (num n) → n
        if (!node.children.empty() && node.children[0].is_integer()) {
            return node.children[0];
        }

        return node;
    }

    if (node.is_list("var")) {
        // (var c) → symbol, where c is a character (A-Z) or string (AA, AB, ...)
        if (!node.children.empty() && node.children[0].is_character()) {
            return lower_var(node.children[0].char_val);
        }

        if (!node.children.empty() && node.children[0].is_string()) {
            return AstNode::make_symbol(node.children[0].str_val);
        }

        return node;
    }

    if (node.is_list("chr-ascii")) {
        // (chr-ascii c) → (chr c) with ¥/‾ substitution
        if (!node.children.empty() && node.children[0].is_character()) {
            return lower_chr_ascii(node.children[0].char_val);
        }

        return node;
    }

    if (node.is_list("chr-sjis1")) {
        // (chr-sjis1 i) → (chr decoded)
        if (!node.children.empty() && node.children[0].is_integer()) {
            return lower_chr_sjis1(node.children[0].int_val, cs);
        }

        return node;
    }

    if (node.is_list("chr-sjis2")) {
        // (chr-sjis2 j1 j2) → (chr decoded) or (chr-raw j1 j2)
        std::vector<int> j;

        for (const auto& child : node.children) {

            if (child.is_integer()) {
                j.push_back(child.int_val);
            }
        }

        return lower_chr_sjis2(j, cfg, cs);
    }

    if (node.is_list("chr-sjis2+")) {
        // (chr-sjis2+ j1 j2) → (chr-raw j1 j2) always
        std::vector<int> j;

        for (const auto& child : node.children) {

            if (child.is_integer()) {
                j.push_back(child.int_val);
            }
        }

        return lower_chr_sjis2_plus(j);
    }

    if (node.is_list("param")) {
        // (param p) → lower p
        if (node.children.size() == 1) {
            return lower_impl(node.children[0], cfg, cs);
        }

        return node;
    }

    if (node.is_list()) {
        // check for (tag (?) body...) pattern, strip empty conditions
        if (node.children.size() >= 1 && node.children[0].is_list("?") &&
            node.children[0].children.empty()) {

            std::vector<AstNode> lowered;
            auto tag_node = lower_impl(AstNode::make_symbol(node.tag), cfg, cs);

            for (size_t i = 1; i < node.children.size(); i++) {
                lowered.push_back(lower_impl(node.children[i], cfg, cs));
            }

            return AstNode::make_list(node.tag, std::move(lowered));
        }

        // check for (cmd (params p...)) pattern + flatten params
        if (node.children.size() >= 1) {

            for (size_t i = 0; i < node.children.size(); i++) {

                if (node.children[i].is_list("params")) {
                    // flatten: replace params node with its children
                    std::vector<AstNode> lowered;

                    for (size_t j = 0; j < i; j++) {
                        lowered.push_back(lower_impl(node.children[j], cfg, cs));
                    }

                    for (const auto& param_child : node.children[i].children) {
                        lowered.push_back(lower_impl(param_child, cfg, cs));
                    }

                    for (size_t j = i + 1; j < node.children.size(); j++) {
                        lowered.push_back(lower_impl(node.children[j], cfg, cs));
                    }

                    return AstNode::make_list(node.tag, std::move(lowered));
                }
            }
        }

        // generic: recurse into children
        std::vector<AstNode> lowered;

        for (const auto& child : node.children) {
            lowered.push_back(lower_impl(child, cfg, cs));
        }

        return AstNode::make_list(node.tag, std::move(lowered));
    }

    // atoms pass through unchanged
    return node;
}

// --- resolve ---

static std::string resolve_cmd(char32_t c, bool do_resolve) {
    int i = static_cast<int>(c);

    if (!do_resolve) {
        return "cmd:" + std::to_string(i);
    }

    switch (i) {
        case 0xA5: return "text-break";
        case 0xA6: return "text-frame";
        case 0xA7: return "text-pos";
        case 0xA8: return "text-color";
        case 0xA9: return "text-delay";
        case 0xAA: return "text-reset";
        case 0xAB: return "wait";
        case 0xAC: return "delay";
        case 0xAD: return "menu1";
        case 0xAE: return "menu2";
        case 0xAF: return "seg-call";
        case 0xB0: return "exec-file";
        case 0xB1: return "mes-jump";
        case 0xB2: return "branch-random";
        case 0xB3: return "branch-index";
        case 0xB4: return "branch-var";
        case 0xB5: return "branch-reg";
        case 0xB7: return "mouse";
        case 0xB9: return "define-proc";
        case 0xBA: return "proc";
        case 0xBB: return "repeat";
        case 0xBC: return "if";
        case 0xBD: return "when";
        case 0xBE: return "flag-save";
        case 0xBF: return "flag-load";
        case 0xC0: return "mes-load?";
        case 0xC8: return "load-mem";
        case 0xC9: return "image-file";
        case 0xCA: return "print-var";
        case 0xCD: return "exec-mem";
        case 0xCF: return "image-mem";
        case 0xD0: return "sound";
        case 0xD5: return "decrypt";
        case 0xD6: return "nop:" + std::to_string(i);
        case 0xD7: return "nop:" + std::to_string(i);
        case 0xD8: return "nop:" + std::to_string(i);
        default:   return "cmd:" + std::to_string(i);
    }
}

AstNode resolve(const AstNode& node, const Config& cfg) {
    return resolve_impl(node, cfg);
}

static AstNode resolve_impl(const AstNode& node, const Config& cfg) {
    if (node.is_list("cmd")) {
        // (cmd c) → resolved command name or cmd:N
        if (!node.children.empty() && node.children[0].is_character()) {
            char32_t c = node.children[0].char_val;
            std::string name = resolve_cmd(c, cfg.resolve);
            return AstNode::make_symbol(name);
        }
    }

    if (node.is_list()) {
        // for empty-tag lists (from parser: ((cmd c) args...))
        // resolve the cmd child, then use its name as the list tag
        if (node.tag.empty() && !node.children.empty() && node.children[0].is_list("cmd")) {
            auto resolved_cmd = resolve_impl(node.children[0], cfg);

            std::vector<AstNode> new_children;

            for (size_t i = 1; i < node.children.size(); i++) {
                new_children.push_back(resolve_impl(node.children[i], cfg));
            }

            return AstNode::make_list(resolved_cmd.str_val, std::move(new_children));
        }

        // recurse into children
        std::vector<AstNode> new_children;

        for (const auto& child : node.children) {
            new_children.push_back(resolve_impl(child, cfg));
        }

        return AstNode::make_list(node.tag, std::move(new_children));
    }

    return node;
}

// --- fuse ---

AstNode fuse(const AstNode& node, const Config& cfg, const Charset& cs) {
    // apply all 9 fuse passes in order
    auto result = node;
    result = fuse_arg(result, cs);
    result = fuse_str(result, cs);
    result = fuse_text(result);
    result = fuse_text_break(result);
    result = fuse_text_proc(result, cfg);
    result = fuse_text_raw(result);
    result = fuse_text_pos(result);
    result = fuse_text_color(result);
    result = fuse_meta(result, cfg);
    return result;
}

// --- fuse passes ---

// helper: convert a char to SJIS bytes for encoding back
static std::vector<uint8_t> char_to_sjis_bytes(char32_t c, const Charset& cs) {
    auto opt = cs.char_to_sjis(c);

    if (opt.has_value()) {
        std::vector<uint8_t> result;

        for (int b : *opt) {

            if (b == 0 && !result.empty()) {
                break; // trailing 0 means single-byte char (half-width katakana, ASCII, etc.)
            }

            result.push_back(static_cast<uint8_t>(b));
        }

        return result;
    }

    return {};
}

static AstNode fuse_arg(const AstNode& node, const Charset& cs) {
    // fuse-arg: (arg chr1 chr2...) → string or bytes
    if (!node.is_list()) {
        return node;
    }

    std::vector<AstNode> new_children;

    for (size_t i = 0; i < node.children.size(); i++) {
        auto child = fuse_arg(node.children[i], cs);

        if (child.is_list("arg")) {
            // check if any chr-byte present
            bool has_chr_byte = false;

            for (const auto& c : child.children) {

                if (c.is_list("chr-byte")) {
                    has_chr_byte = true;
                    break;
                }
            }

            if (!has_chr_byte) {
                // convert to string: collect characters
                std::string s;

                for (const auto& c : child.children) {

                    if (c.is_list("chr") && !c.children.empty() && c.children[0].is_character()) {
                        s += char32_to_utf8(c.children[0].char_val);
                    } else if (c.is_list("chr-ascii") && !c.children.empty() && c.children[0].is_character()) {
                        s += char32_to_utf8(c.children[0].char_val);
                    } else if (c.is_string()) {
                        s += c.str_val;
                    }
                }

                new_children.push_back(AstNode::make_string(s));
            } else {
                // convert to bytes: need to encode chars back to SJIS
                std::vector<uint8_t> bytes;

                for (const auto& c : child.children) {

                    if (c.is_list("chr-byte") && !c.children.empty() && c.children[0].is_character()) {
                        bytes.push_back(static_cast<uint8_t>(c.children[0].char_val));
                    } else if (c.is_list("chr") && !c.children.empty() && c.children[0].is_character()) {
                        auto sjis = char_to_sjis_bytes(c.children[0].char_val, cs);

                        for (uint8_t b : sjis) {
                            bytes.push_back(b);
                        }
                    }
                }

                // represent as a bytes node
                std::vector<AstNode> byte_nodes;

                for (uint8_t b : bytes) {
                    byte_nodes.push_back(AstNode::make_integer(b));
                }

                new_children.push_back(AstNode::make_list("bytes", std::move(byte_nodes)));
            }

            continue;
        }

        new_children.push_back(std::move(child));
    }

    return AstNode::make_list(node.tag, std::move(new_children));
}

// helper: convert SJIS byte pair to integer form
static int sjis_to_integer_sjis(const std::vector<int>& pair) {
    return Charset::sjis_to_integer(pair);
}

static AstNode fuse_str(const AstNode& node, const Charset& cs) {
    // fuse-str: (chrs! chr1 chr2...) → (str "text" ...)
    if (!node.is_list()) {
        return node;
    }

    std::vector<AstNode> new_children;

    for (size_t i = 0; i < node.children.size(); i++) {
        auto child = fuse_str(node.children[i], cs);

        if (child.is_list("chrs!")) {
            // process children: consecutive chr → string, chr-raw → quoted integer
            std::vector<AstNode> str_parts;
            size_t j = 0;

            while (j < child.children.size()) {

                // consecutive chr nodes → string
                if (child.children[j].is_list("chr")) {
                    std::string s;

                    while (j < child.children.size() && child.children[j].is_list("chr")) {

                        if (!child.children[j].children.empty() &&
                            child.children[j].children[0].is_character()) {
                            s += char32_to_utf8(child.children[j].children[0].char_val);
                        }

                        j++;
                    }

                    if (!s.empty()) {
                        str_parts.push_back(AstNode::make_string(s));
                    }

                    continue;
                }

                // chr-raw → quoted SJIS integer symbol
                if (child.children[j].is_list("chr-raw")) {
                    std::vector<int> pair;

                    for (const auto& b : child.children[j].children) {

                        if (b.is_integer()) {
                            pair.push_back(b.int_val);
                        }
                    }

                    int sjis_int = sjis_to_integer_sjis(pair);
                    str_parts.push_back(AstNode::make_quote(
                        AstNode::make_symbol(std::to_string(sjis_int))));
                    j++;
                    continue;
                }

                // other: pass through
                str_parts.push_back(std::move(child.children[j]));
                j++;
            }

            new_children.push_back(AstNode::make_list("str", std::move(str_parts)));
            continue;
        }

        new_children.push_back(std::move(child));
    }

    return AstNode::make_list(node.tag, std::move(new_children));
}

static AstNode fuse_text(const AstNode& node) {
    // fuse-text: (chrs chr1 chr2...) → (text "line1") (text "line2")
    if (!node.is_list()) {
        return node;
    }

    std::vector<AstNode> new_children;

    for (size_t i = 0; i < node.children.size(); i++) {
        auto child = fuse_text(node.children[i]);

        if (child.is_list("chrs")) {
            size_t j = 0;

            while (j < child.children.size()) {

                // consecutive chr nodes → text string, split on newlines
                if (child.children[j].is_list("chr")) {
                    std::string text;

                    while (j < child.children.size() && child.children[j].is_list("chr")) {

                        if (!child.children[j].children.empty() &&
                            child.children[j].children[0].is_character()) {
                            text += char32_to_utf8(child.children[j].children[0].char_val);
                        }

                        j++;
                    }

                    // split on newlines
                    if (!text.empty()) {
                        size_t pos = 0;

                        while (pos < text.size()) {
                            size_t nl = text.find('\n', pos);

                            if (nl == std::string::npos) {
                                new_children.push_back(AstNode::make_list("text", {
                                    AstNode::make_string(text.substr(pos))
                                }));
                                break;
                            }

                            new_children.push_back(AstNode::make_list("text", {
                                AstNode::make_string(text.substr(pos, nl - pos + 1))
                            }));
                            pos = nl + 1;
                        }
                    }

                    continue;
                }

                // consecutive chr-raw → (text-raw sjis-integers...)
                if (child.children[j].is_list("chr-raw")) {
                    std::vector<AstNode> raw_ints;

                    while (j < child.children.size() && child.children[j].is_list("chr-raw")) {
                        std::vector<int> pair;

                        for (const auto& b : child.children[j].children) {

                            if (b.is_integer()) {
                                pair.push_back(b.int_val);
                            }
                        }

                        raw_ints.push_back(AstNode::make_integer(sjis_to_integer_sjis(pair)));
                        j++;
                    }

                    new_children.push_back(AstNode::make_list("text-raw", std::move(raw_ints)));
                    continue;
                }

                // other: pass through
                new_children.push_back(std::move(child.children[j]));
                j++;
            }

            continue;
        }

        new_children.push_back(std::move(child));
    }

    return AstNode::make_list(node.tag, std::move(new_children));
}

static AstNode fuse_text_break(const AstNode& node) {
    // fuse-text-break:
    //   (text-break) → (text 'br)
    //   (text t...) (text-break) → (text t... 'br)
    if (!node.is_list()) {
        return node;
    }

    std::vector<AstNode> new_children;
    bool just_merged = false;

    for (size_t i = 0; i < node.children.size(); i++) {
        auto child = fuse_text_break(node.children[i]);

        if (child.is_list("text-break") && child.children.empty()) {

            // merge with preceding (text ...) only if we didn't just merge.
            // matches racket behavior: (text t...) (text-break) merges once,
            // a following (text-break) becomes standalone (text 'br).
            if (!just_merged && !new_children.empty() && new_children.back().is_list("text")) {
                new_children.back().children.push_back(
                    AstNode::make_quote(AstNode::make_symbol("br")));
                just_merged = true;
            } else {
                // standalone: (text 'br)
                new_children.push_back(AstNode::make_list("text", {
                    AstNode::make_quote(AstNode::make_symbol("br"))
                }));
                just_merged = false;
            }

            continue;
        }

        just_merged = false;
        new_children.push_back(std::move(child));
    }

    return AstNode::make_list(node.tag, std::move(new_children));
}

static AstNode fuse_text_proc(const AstNode& node, const Config& cfg) {
    // fuse-text-proc: (proc N) where N is protagonist → (text N) or (text (proc N))
    if (!node.is_list()) {
        return node;
    }

    std::vector<AstNode> new_children;

    for (size_t i = 0; i < node.children.size(); i++) {
        auto child = fuse_text_proc(node.children[i], cfg);

        if (child.is_list("proc") && child.children.size() == 1) {

            if (child.children[0].is_integer() && cfg.is_protag(child.children[0].int_val)) {
                new_children.push_back(AstNode::make_list("text", {
                    AstNode::make_integer(child.children[0].int_val)
                }));
                continue;
            }

            if (child.children[0].is_symbol()) {
                // check if it's a protag variable
                const std::string& name = child.children[0].str_val;

                if (name.size() == 1 && cfg.is_protag(name[0])) {
                    new_children.push_back(AstNode::make_list("text", {
                        AstNode::make_list("proc", {child.children[0]})
                    }));
                    continue;
                }
            }
        }

        new_children.push_back(std::move(child));
    }

    return AstNode::make_list(node.tag, std::move(new_children));
}

static AstNode fuse_text_raw(const AstNode& node) {
    // fuse-text-raw: merge (text-raw i) with adjacent (text ...) using quoted symbol
    if (!node.is_list()) {
        return node;
    }

    std::vector<AstNode> new_children;

    for (size_t i = 0; i < node.children.size(); i++) {
        auto child = fuse_text_raw(node.children[i]);

        // (text-raw i) followed by (text t...)
        if (child.is_list("text-raw") && child.children.size() == 1 &&
            i + 1 < node.children.size()) {

            auto next = fuse_text_raw(node.children[i + 1]);

            if (next.is_list("text")) {
                int sjis_int = child.children[0].int_val;
                std::vector<AstNode> merged;
                merged.push_back(AstNode::make_quote(AstNode::make_symbol(std::to_string(sjis_int))));

                for (auto& tc : next.children) {
                    merged.push_back(std::move(tc));
                }

                i++; // consumed the (text ...) node

                // absorb any trailing (text-raw j) nodes
                while (i + 1 < node.children.size()) {
                    auto& peek_after = node.children[i + 1];

                    if (!peek_after.is_list("text-raw") || peek_after.children.size() != 1) {
                        break;
                    }

                    int sjis_int2 = peek_after.children[0].int_val;
                    merged.push_back(AstNode::make_quote(
                        AstNode::make_symbol(std::to_string(sjis_int2))));
                    i++; // consumed trailing text-raw
                }

                new_children.push_back(AstNode::make_list("text", std::move(merged)));
                continue;
            }

            // no text follows, check previous
            new_children.push_back(std::move(child));
            continue;
        }

        // (text t...) followed by one or more (text-raw i)
        if (child.is_list("text") && i + 1 < node.children.size()) {

            while (i + 1 < node.children.size()) {
                auto& peek_next = node.children[i + 1];

                if (!peek_next.is_list("text-raw") || peek_next.children.size() != 1) {
                    break;
                }

                int sjis_int = peek_next.children[0].int_val;
                child.children.push_back(
                    AstNode::make_quote(AstNode::make_symbol(std::to_string(sjis_int))));
                i++; // consumed this text-raw
            }

            new_children.push_back(std::move(child));
            continue;
        }

        new_children.push_back(std::move(child));
    }

    return AstNode::make_list(node.tag, std::move(new_children));
}

static AstNode fuse_text_pos(const AstNode& node) {
    // fuse-text-pos: (text-pos p...) (text t...) → (text #:pos 'p t...)
    if (!node.is_list()) {
        return node;
    }

    std::vector<AstNode> new_children;

    for (size_t i = 0; i < node.children.size(); i++) {
        auto child = fuse_text_pos(node.children[i]);

        if (child.is_list("text-pos") && i + 1 < node.children.size()) {
            auto next = fuse_text_pos(node.children[i + 1]);

            if (next.is_list("text")) {
                std::vector<AstNode> merged;
                merged.push_back(AstNode::make_keyword("pos"));

                // quote the position params as a list
                std::vector<AstNode> pos_params;

                for (auto& p : child.children) {
                    pos_params.push_back(std::move(p));
                }

                merged.push_back(AstNode::make_quote(
                    AstNode::make_list("", std::move(pos_params))));

                for (auto& tc : next.children) {
                    merged.push_back(std::move(tc));
                }

                new_children.push_back(AstNode::make_list("text", std::move(merged)));
                i++; // skip next
                continue;
            }

            // no text follows, keep as-is
            new_children.push_back(std::move(child));
            new_children.push_back(std::move(next));
            i++;
            continue;
        }

        new_children.push_back(std::move(child));
    }

    return AstNode::make_list(node.tag, std::move(new_children));
}

static AstNode fuse_text_color(const AstNode& node) {
    // fuse-text-color: (text-color c) (text t...) → (text #:col c t...)
    if (!node.is_list()) {
        return node;
    }

    std::vector<AstNode> new_children;

    for (size_t i = 0; i < node.children.size(); i++) {
        auto child = fuse_text_color(node.children[i]);

        if (child.is_list("text-color") && !child.children.empty() &&
            i + 1 < node.children.size()) {

            auto next = fuse_text_color(node.children[i + 1]);

            if (next.is_list("text")) {
                std::vector<AstNode> merged;
                merged.push_back(AstNode::make_keyword("col"));
                merged.push_back(std::move(child.children[0]));

                for (auto& tc : next.children) {
                    merged.push_back(std::move(tc));
                }

                new_children.push_back(AstNode::make_list("text", std::move(merged)));
                i++; // skip next
                continue;
            }

            // no text follows, keep both
            new_children.push_back(std::move(child));
            new_children.push_back(std::move(next));
            i++;
            continue;
        }

        new_children.push_back(std::move(child));
    }

    return AstNode::make_list(node.tag, std::move(new_children));
}

static AstNode fuse_meta(const AstNode& node, const Config& cfg) {
    // fuse-meta: wrap with (meta (engine 'ADV) (charset ...) [(extraop ...)])
    if (!node.is_list("mes")) {
        return node;
    }

    std::vector<AstNode> meta_entries;

    // engine
    meta_entries.push_back(AstNode::make_list("engine", {
        AstNode::make_quote(AstNode::make_symbol("ADV"))
    }));

    // charset
    meta_entries.push_back(AstNode::make_list("charset", {
        AstNode::make_string(cfg.charset_name)
    }));

    // extraop (only if true)
    if (cfg.extra_op) {
        meta_entries.push_back(AstNode::make_list("extraop", {
            AstNode::make_boolean(true)
        }));
    }

    // prepend meta to mes children
    std::vector<AstNode> new_children;
    new_children.push_back(AstNode::make_list("meta", std::move(meta_entries)));

    for (const auto& child : node.children) {
        new_children.push_back(child);
    }

    return AstNode::make_list("mes", std::move(new_children));
}

} // namespace adv
