// lexstate.cpp -- LexState::load (populate the in-memory input line source).
#include "specparse/lexstate.hpp"

namespace x13 {

void LexState::load(const std::string& text) {
    input_lines.clear();
    read_idx = 0;
    std::string cur;
    for (char c : text) {
        if (c == '\n') {
            if (!cur.empty() && cur.back() == '\r') cur.pop_back();
            input_lines.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    // Trailing line without a final newline.
    if (!cur.empty()) {
        if (cur.back() == '\r') cur.pop_back();
        input_lines.push_back(cur);
    }
}

} // namespace x13
