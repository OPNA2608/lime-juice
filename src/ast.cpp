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
