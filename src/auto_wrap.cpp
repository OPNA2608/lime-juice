#include "auto_wrap.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>


// display length of text content.
// escape sequences (\" and \\) count as 1 display character each.
static size_t display_len(const std::string& text) {
    size_t len = 0;

    for (size_t i = 0; i < text.size(); i++) {

        if (text[i] == '\\' && i + 1 < text.size()) {
            i++;
        }

        len++;
    }

    return len;
}

// trim trailing spaces from a string
static std::string rtrim(const std::string& s) {
    size_t end = s.find_last_not_of(' ');
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

// word-wrap a string into lines of at most 'width' characters.
// breaks at the last word boundary (space) before the width limit.
// no padding is added; 'br markers handle line breaks instead.
static std::vector<std::string> format_line(const std::string& text, int width) {

    if (width <= 0) {
        return {text};
    }

    std::vector<std::string> lines;
    std::string remaining = text;

    while (!remaining.empty()) {

        // if the rest fits within width, it becomes the final line
        if (static_cast<int>(remaining.size()) <= width) {
            lines.push_back(remaining);
            break;
        }

        // check if there's a space right at the width boundary,
        // meaning the first WIDTH chars form complete words
        if (remaining[width] == ' ') {
            lines.push_back(remaining.substr(0, width));
            remaining = remaining.substr(width + 1);
            continue;
        }

        // find the last space within the first WIDTH characters
        int break_pos = -1;

        for (int i = width - 1; i >= 0; i--) {

            if (remaining[i] == ' ') {
                break_pos = i;
                break;
            }
        }

        if (break_pos > 0) {
            // break at the last space, consume the space itself
            lines.push_back(remaining.substr(0, break_pos));
            remaining = remaining.substr(break_pos + 1);
        } else {
            // no space found within width, force break at width
            lines.push_back(remaining.substr(0, width));
            remaining = remaining.substr(width);
        }
    }

    return lines;
}

// check if a node is 'br (a quote wrapping the symbol "br")
static bool is_br_quote(const AstNode& node) {
    return node.is_quote() && !node.children.empty() &&
           node.children[0].is_symbol("br");
}

// scan all define-proc blocks in the AST to build a proc_number -> width map
static std::unordered_map<int, int> scan_proc_widths(const AstNode& ast) {
    std::unordered_map<int, int> widths;

    std::function<void(const AstNode&)> scan = [&](const AstNode& node) {

        if (node.is_list("define-proc") && !node.children.empty() &&
            node.children[0].is_integer()) {

            int proc_num = node.children[0].int_val;

            // find the LAST text-frame in this proc (the one that remains active)
            int last_x1 = -1, last_x2 = -1;
            std::function<void(const AstNode&)> find_tf = [&](const AstNode& n) {

                if (n.is_list("text-frame") && n.children.size() >= 4 &&
                    n.children[0].is_integer() && n.children[2].is_integer()) {
                    last_x1 = n.children[0].int_val;
                    last_x2 = n.children[2].int_val;
                }

                for (const auto& c : n.children) {
                    find_tf(c);
                }
            };

            find_tf(node);

            if (last_x1 >= 0 && last_x2 >= 0) {
                widths[proc_num] = (last_x2 - last_x1) + 1;
            }
        }

        for (const auto& c : node.children) {
            scan(c);
        }
    };

    scan(ast);
    return widths;
}

// attempt to wrap a single text node into multiple text nodes.
// returns empty vector if no wrapping was needed or possible.
static std::vector<AstNode> try_wrap_text(const AstNode& text_node, int width) {
    // find the string child and check for 'br suffix
    int string_idx = -1;
    bool has_br = false;

    for (size_t i = 0; i < text_node.children.size(); i++) {

        if (text_node.children[i].is_string() && string_idx < 0) {
            string_idx = static_cast<int>(i);
        } else if (is_br_quote(text_node.children[i])) {
            has_br = true;
        }
    }

    if (string_idx < 0) {
        return {};
    }

    // skip positional text nodes (#:pos has different semantics)
    for (int i = 0; i < string_idx; i++) {

        if (text_node.children[i].is_keyword() &&
            text_node.children[i].str_val == "pos") {
            return {};
        }
    }

    const std::string& content = text_node.children[string_idx].str_val;

    if (display_len(content) <= static_cast<size_t>(width)) {
        return {};
    }

    auto lines = format_line(content, width);

    if (lines.size() <= 1) {
        return {};
    }

    // collect prefix nodes (everything before the string, e.g. #:col 15)
    std::vector<AstNode> prefix;

    for (int i = 0; i < string_idx; i++) {
        prefix.push_back(text_node.children[i]);
    }

    // build replacement text nodes
    std::vector<AstNode> wrapped;

    for (size_t k = 0; k < lines.size(); k++) {
        std::vector<AstNode> kids;
        bool is_first = (k == 0);
        bool is_last = (k == lines.size() - 1);

        // prefix (e.g. #:col 15) on first line only
        if (is_first) {

            for (const auto& p : prefix) {
                kids.push_back(p);
            }
        }

        // trim trailing spaces from the line
        std::string trimmed = rtrim(lines[k]);
        kids.push_back(AstNode::make_string(trimmed));

        // 'br logic (matches format-line.sh behavior):
        // a line that exactly fills the frame width AND had no
        // trailing spaces trimmed auto-advances the cursor, so
        // 'br would cause a double line break.
        // shorter or trimmed lines need an explicit 'br.
        // last line: 'br is stripped unless the original had one.
        bool needs_br = false;

        if (!is_last) {
            bool exact_width = (static_cast<int>(lines[k].size()) == width);
            bool was_trimmed = (lines[k] != trimmed);
            needs_br = !exact_width || was_trimmed;
        } else {
            needs_br = has_br;
        }

        if (needs_br) {
            kids.push_back(
                AstNode::make_quote(AstNode::make_symbol("br")));
        }

        wrapped.push_back(AstNode::make_list("text", std::move(kids)));
    }

    return wrapped;
}

// recursively walk the AST, tracking text-frame width and wrapping text nodes.
// current_width is passed by reference so width changes at any nesting level
// propagate globally (matching the line-by-line behavior of wrap-lines.py).
static void process_children(std::vector<AstNode>& children, int& width,
                             const std::unordered_map<int, int>& proc_widths,
                             bool in_define_proc) {

    std::vector<AstNode> result;

    for (auto& child : children) {

        // track text-frame width from direct commands (not inside define-proc)
        if (!in_define_proc && child.is_list("text-frame") &&
            child.children.size() >= 4 &&
            child.children[0].is_integer() && child.children[2].is_integer()) {

            width = (child.children[2].int_val - child.children[0].int_val) + 1;
            result.push_back(std::move(child));
            continue;
        }

        // track width from proc calls
        if (!in_define_proc && child.is_list("proc") &&
            !child.children.empty() && child.children[0].is_integer()) {

            auto it = proc_widths.find(child.children[0].int_val);

            if (it != proc_widths.end()) {
                width = it->second;
            }

            result.push_back(std::move(child));
            continue;
        }

        // try wrapping text nodes (outside define-proc bodies)
        if (!in_define_proc && child.is_list("text") && width > 0) {
            auto wrapped = try_wrap_text(child, width);

            if (!wrapped.empty()) {

                for (auto& w : wrapped) {
                    result.push_back(std::move(w));
                }

                continue;
            }
        }

        // recurse into list children
        if (child.kind == AstNode::Kind::List && !child.children.empty()) {
            bool child_is_dp = child.is_list("define-proc");
            process_children(child.children, width, proc_widths,
                             in_define_proc || child_is_dp);
        }

        result.push_back(std::move(child));
    }

    children = std::move(result);
}

void auto_wrap_ast(AstNode& ast) {
    auto proc_widths = scan_proc_widths(ast);
    int current_width = 80;
    process_children(ast.children, current_width, proc_widths, false);
}
