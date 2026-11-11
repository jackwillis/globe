// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Word wrap for information cards: breaks each card line at spaces so it fits a pixel width the
// platform measures (Quickdraw's TextWidth, Xlib's XTextWidth). Fixed storage, no allocation; the
// rows are views into the card's own text. Text past the capacity is dropped, never overrun.
#pragma once
#include "globe/countries.hpp"
#include <array>
#include <string_view>
namespace globe {
struct WrappedCard {
    static constexpr unsigned kCapacity = 48;
    std::array<std::string_view, kCapacity> rows{};
    unsigned count = 0;
};
// `width(std::string_view) -> int` is the platform's text measure; `max_width` is in the same unit.
template <class Measure>
WrappedCard wrap_card(const InfoCard& card, int max_width, Measure&& width) {
    WrappedCard out;
    const auto push = [&](std::string_view row) {
        if (out.count < WrappedCard::kCapacity)
            out.rows[out.count++] = row;
    };
    const unsigned lines = card.line_count < card.lines.size()
                               ? card.line_count
                               : static_cast<unsigned>(card.lines.size());
    for (unsigned i = 0; i < lines; ++i) {
        std::string_view rest = card.lines[i];
        if (rest.empty())
            push(rest);
        while (!rest.empty()) {
            while (rest.size() > 1 && rest.front() == ' ')
                rest.remove_prefix(1);
            // The longest prefix, ending at a space or the end of text, that fits.
            std::size_t fit = 0, end = 0;
            while (end < rest.size()) {
                std::size_t next = rest.find(' ', end);
                if (next == std::string_view::npos)
                    next = rest.size();
                if (width(rest.substr(0, next)) > max_width)
                    break;
                fit = next;
                end = next + 1;
            }
            if (fit == 0) { // one word wider than the pane: break it by characters
                fit = 1;
                while (fit < rest.size() && width(rest.substr(0, fit + 1)) <= max_width)
                    ++fit;
            }
            push(rest.substr(0, fit));
            rest.remove_prefix(fit);
        }
    }
    return out;
}
} // namespace globe
