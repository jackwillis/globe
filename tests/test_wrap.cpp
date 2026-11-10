// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/wrap.hpp"
using namespace globe;
static int chars(std::string_view s) { return static_cast<int>(s.size()); }
int main() {
    InfoCard card;
    card.lines[0] = "The quick brown fox jumps";
    card.lines[1] = "";
    card.lines[2] = "Supercalifragilistic";
    card.lines[3] = "short";
    card.line_count = 4;
    const auto w = wrap_card(card, 10, chars);
    // "The quick" / "brown fox" / "jumps" / "" / "Supercalif" / "ragilistic" / "short"
    CHECK(w.count == 7);
    CHECK(w.rows[0] == "The quick");
    CHECK(w.rows[1] == "brown fox");
    CHECK(w.rows[2] == "jumps");
    CHECK(w.rows[3].empty());
    CHECK(w.rows[4] == "Supercalif");
    CHECK(w.rows[5] == "ragilistic");
    CHECK(w.rows[6] == "short");
    for (unsigned i = 0; i < w.count; ++i)
        CHECK(chars(w.rows[i]) <= 10);
    const auto wide = wrap_card(card, 200, chars);
    CHECK(wide.count == 4 && wide.rows[0] == card.lines[0]);
    CHECK(wrap_card(card, 1, chars).count <= WrappedCard::kCapacity);
    return TEST_MAIN_RESULT();
}
