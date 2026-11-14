// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/controls.hpp"
#include "globe/wrap.hpp"
#include <string>
using namespace globe;
namespace {
bool has(const InfoCard& card, std::string_view text) {
    for (unsigned i = 0; i < card.line_count; ++i)
        if (card.lines[i].find(text) != std::string_view::npos)
            return true;
    return false;
}
} // namespace
int main() {
    using Overlay = KeyBinding::Overlay;
    // The table is the key binding: each typed character, in either case, means what its row says,
    // and every character no row lists means nothing.
    for (int ch = -1; ch < 300; ++ch) {
        int owners = 0;
        for (unsigned i = 0; i < control_count(); ++i) {
            const auto& row = controls()[i];
            const auto lower = ch >= 'A' && ch <= 'Z' ? ch + 32 : ch;
            if (ch < 0 || ch > 127 ||
                row.chars.find(static_cast<char>(lower)) == std::string_view::npos)
                continue;
            ++owners;
            const auto b = binding_for_char(ch);
            CHECK(b.command == row.command && b.overlay == row.overlay);
        }
        CHECK(owners <= 1); // one character, one meaning
        if (owners == 0) {
            const auto b = binding_for_char(ch);
            CHECK(!b.command && b.overlay == Overlay::Nothing);
        }
    }
    for (unsigned i = 0; i < control_count(); ++i) {
        const auto& row = controls()[i];
        CHECK(!row.keys.empty() && !row.what.empty());
        CHECK(row.classic_mac || row.x11);
        // A row that lists characters does something with them, and in lower case.
        CHECK(row.chars.empty() || row.command || row.overlay != Overlay::Nothing);
        for (const char c : row.chars)
            CHECK(!(c >= 'A' && c <= 'Z'));
    }
    // Every command a key sends is documented: Help names each shortcut the front ends honor.
    for (int ch = 1; ch < 128; ++ch) {
        const auto b = binding_for_char(ch);
        if (!b.command && b.overlay == Overlay::Nothing)
            continue;
        bool listed = false;
        for (unsigned i = 0; i < control_count(); ++i)
            listed |= controls()[i].chars.find(static_cast<char>(ch)) != std::string_view::npos;
        CHECK(listed);
    }
    CHECK(binding_for_char('h').overlay == Overlay::Help);
    CHECK(binding_for_char('H').overlay == Overlay::Help);
    CHECK(binding_for_char('?').overlay == Overlay::Help);
    CHECK(!binding_for_char('h').command);

    // Starting a measurement (not leaving it, not another overlay) replaces Help.
    CHECK(measure_replaces_help(Overlay::Measure, true));
    CHECK(!measure_replaces_help(Overlay::Measure, false));
    CHECK(!measure_replaces_help(Overlay::Night, true) &&
          !measure_replaces_help(Overlay::Help, true));
    CHECK(!measure_replaces_help(Overlay::Nothing, true));

    // What each front end shows is its own: the wheel, F2 and bare Q/Esc are X11; Command-Q is
    // Classic. Shared keys are in both.
    const auto mac = help_card(Frontend::ClassicMac), x11 = help_card(Frontend::X11);
    CHECK(mac.title == "Controls" && x11.title == "Controls");
    for (const auto* both : {"Drag", "Double-click", "H or ?", "G: graticule", "D: night side",
                             "M: measure", "S: switch Map / Relief", "Page Up / Page Down"}) {
        CHECK(has(mac, both) && has(x11, both));
    }
    CHECK(has(x11, "Mouse wheel") && has(x11, "F2") && has(x11, "Q or Esc"));
    CHECK(!has(mac, "Mouse wheel") && !has(mac, "F2") && !has(mac, "Q or Esc"));
    CHECK(has(mac, "Command-Q") && !has(x11, "Command-Q"));
    CHECK(has(mac, "Esc: hide this help") && !has(x11, "Esc: hide this help"));
    // Lines are "keys: what", nothing was dropped for want of room, and the text is stable.
    for (const auto frontend : {Frontend::ClassicMac, Frontend::X11}) {
        const auto card = help_card(frontend);
        unsigned expected = 0;
        for (unsigned i = 0; i < control_count(); ++i)
            expected += applies_to(controls()[i], frontend);
        CHECK(card.line_count == expected && expected <= card.lines.size());
        for (unsigned i = 0; i < card.line_count; ++i)
            CHECK(card.lines[i].find(": ") != std::string_view::npos);
        const auto again = help_card(frontend);
        CHECK(again.line_count == card.line_count && again.lines[3] == card.lines[3]);
    }

    // Presentation at the smallest information pane (a 640-pixel window less the 220-pixel sidebar
    // and its scroll bar leaves about 190 pixels; 6 pixels a character is a generous Geneva 9):
    // every row wraps into the scroller, whole words, and still fits the wrap capacity.
    const auto six = [](std::string_view t) { return static_cast<int>(t.size()) * 6; };
    for (const int width : {190, 160, 120}) {
        for (const auto frontend : {Frontend::ClassicMac, Frontend::X11}) {
            const auto card = help_card(frontend);
            const auto rows = wrap_card(card, width, six);
            CHECK(rows.count >= card.line_count && rows.count < WrappedCard::kCapacity);
            for (unsigned i = 0; i < rows.count; ++i)
                CHECK(six(rows.rows[i]) <= width);
            // Nothing lost: the key of the last line is on the last rows.
            std::string tail;
            for (unsigned i = 0; i < rows.count; ++i)
                tail += std::string(rows.rows[i]) + " ";
            CHECK(tail.find(std::string(card.lines[card.line_count - 1].substr(0, 4))) !=
                  std::string::npos);
        }
    }
    return 0;
}
