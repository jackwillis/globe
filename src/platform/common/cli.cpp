// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "cli.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>

namespace globe {

bool parse_int_arg(const char* text, int lo, int hi, int& out) {
    if (text == nullptr || lo > hi) {
        return false;
    }
    const bool digit_first = text[0] >= '0' && text[0] <= '9';
    const bool minus_ok = text[0] == '-' && lo < 0 && text[1] >= '0' && text[1] <= '9';
    if (!digit_first && !minus_ok) {
        return false;
    }
    char* end = nullptr;
    errno = 0;
    const long v = std::strtol(text, &end, 10);
    if (errno != 0 || *end != '\0' || v < lo || v > hi) {
        return false;
    }
    out = static_cast<int>(v);
    return true;
}

bool parse_size_arg(const char* text, int max_side, int& width, int& height) {
    if (text == nullptr) {
        return false;
    }
    const char* x = std::strchr(text, 'x');
    if (x == nullptr || x == text || x[1] == '\0') {
        return false;
    }
    char head[16];
    const std::size_t n = static_cast<std::size_t>(x - text);
    if (n >= sizeof head) {
        return false;
    }
    std::memcpy(head, text, n);
    head[n] = '\0';
    int w = 0;
    int h = 0;
    if (!parse_int_arg(head, 1, max_side, w) || !parse_int_arg(x + 1, 1, max_side, h)) {
        return false;
    }
    width = w;
    height = h;
    return true;
}

} // namespace globe
