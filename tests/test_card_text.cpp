// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/card_text.hpp"
#include <string>
using namespace globe;
int main() {
    InfoCard card;
    card.title = "Country";
    card.lines[0] = "Name: Peru";
    card.lines[1] = "Capital: Lima";
    card.line_count = 2;
    char buf[64];
    auto n = card_text(card, buf, sizeof buf, '\r');
    CHECK(std::string(buf, n) == "Country\rName: Peru\rCapital: Lima");
    n = card_text(card, buf, 12, '\n'); // cut, never overrun
    CHECK(n == 12 && std::string(buf, n) == "Country\nName");
    card.line_count = 0;
    n = card_text(card, buf, sizeof buf, '\n');
    CHECK(std::string(buf, n) == "Country");
    card.line_count = 99; // more than the card holds: clamped
    n = card_text(card, buf, sizeof buf, '\n');
    CHECK(n > 0);
    return TEST_MAIN_RESULT();
}
