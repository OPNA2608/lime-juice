#include "ast.h"

bool AstNode::operator==(const AstNode& other) const {
    if (kind != other.kind) {
        return false;
    }

    switch (kind) {
        case Kind::Integer:
            return int_val == other.int_val;
        case Kind::Variable:
            return var_val == other.var_val;
        case Kind::Symbol:
        case Kind::String:
        case Kind::Keyword:
            return str_val == other.str_val;
        case Kind::Character:
            return char_val == other.char_val;
        case Kind::CharRaw:
            return raw_bytes == other.raw_bytes;
        case Kind::DicRef:
            return int_val == other.int_val;
        case Kind::Boolean:
            return bool_val == other.bool_val;
        case Kind::Cut:
            return true;
        case Kind::Quote:
            return children == other.children;
        case Kind::List:
            return tag == other.tag && children == other.children;
    }

    return false;
}
