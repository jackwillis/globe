// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// An information card as plain text for the clipboard: the title, then each line, separated by
// `eol` ('\r' for the Classic Mac scrap, '\n' for X11). No allocation; text that does not fit
// `capacity` is cut, never overrun.
#pragma once
#include "globe/countries.hpp"
#include <cstddef>
namespace globe {
inline std::size_t card_text(const InfoCard& card, char* out, std::size_t capacity, char eol) {
    std::size_t n = 0;
    const auto add = [&](std::string_view text) {
        for (const char c : text)
            if (n < capacity)
                out[n++] = c;
    };
    add(card.title);
    const unsigned lines = card.line_count < card.lines.size()
                               ? card.line_count
                               : static_cast<unsigned>(card.lines.size());
    for (unsigned i = 0; i < lines; ++i) {
        add(std::string_view(&eol, 1));
        add(card.lines[i]);
    }
    return n;
}
} // namespace globe
