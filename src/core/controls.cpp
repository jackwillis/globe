// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/controls.hpp"
#include <array>
#include <cstddef>
namespace globe {
namespace {
using Overlay = KeyBinding::Overlay;
constexpr Overlay kNo = Overlay::Nothing;
constexpr std::optional<Command> kNone{};
// Text is English and ASCII, as in l10n/en.txt; it moves there with the rest of the UI text when
// translation starts (#84).
constexpr std::array kControls{
    Control{true, true, "Drag", "turn the globe", "", kNone, kNo},
    Control{true, true, "Arrow keys", "turn the globe", "", kNone, kNo},
    Control{true, true, "Double-click", "centre and zoom in", "", kNone, kNo},
    Control{true, true, "Click", "select a country or place", "", kNone, kNo},
    Control{false, true, "Mouse wheel", "zoom", "", kNone, kNo},
    Control{true, true, "Page Up / Page Down", "zoom in / out (wheel drivers send these)", "",
            kNone, kNo},
    Control{true, true, "+ or =", "zoom in", "+=", Command::ZoomIn, kNo},
    Control{true, true, "-", "zoom out", "-", Command::ZoomOut, kNo},
    Control{true, true, "R", "reset the view", "r", Command::Reset, kNo},
    Control{true, true, "N", "align north up", "n", Command::AlignNorth, kNo},
    Control{true, true, "T", "trackball drag", "t", Command::ToggleDragMode, kNo},
    Control{true, true, "G", "graticule", "g", Command::ToggleGraticule, kNo},
    Control{true, true, "B", "borders", "b", Command::ToggleBorders, kNo},
    Control{true, true, "S", "switch Map / Relief", "s", Command::CycleStyle, kNo},
    Control{true, true, "D", "night side", "d", std::nullopt, Overlay::Night},
    Control{true, true, "M", "measure; click two points", "m", std::nullopt, Overlay::Measure},
    Control{true, true, "H or ?", "show or hide this help", "h?", std::nullopt, Overlay::Help},
    Control{false, true, "F2", "show the toolbar", "", kNone, kNo},
    Control{false, true, "Q or Esc", "quit (Esc closes help first)", "q\x1b", Command::Quit, kNo},
    Control{true, false, "Esc", "hide this help", "", kNone, kNo},
    Control{true, false, "Command-Q", "quit", "", kNone, kNo},
};
} // namespace
const Control* controls() {
    return kControls.data();
}
unsigned control_count() {
    return static_cast<unsigned>(kControls.size());
}
bool applies_to(const Control& control, Frontend frontend) {
    return frontend == Frontend::ClassicMac ? control.classic_mac : control.x11;
}
KeyBinding binding_for_char(int ch) {
    if (ch >= 'A' && ch <= 'Z')
        ch += 'a' - 'A';
    KeyBinding b;
    if (ch <= 0 || ch > 127)
        return b;
    for (const auto& control : kControls) {
        if (control.chars.find(static_cast<char>(ch)) == std::string_view::npos)
            continue;
        if (control.command || control.overlay != Overlay::Nothing) {
            b.command = control.command;
            b.overlay = control.overlay;
            return b;
        }
    }
    return b;
}
InfoCard help_card(Frontend frontend) {
    // Fixed storage for the composed lines, one set per front end, rebuilt identically each call.
    static char text[2][20][96];
    auto& lines = text[frontend == Frontend::ClassicMac ? 0 : 1];
    InfoCard card;
    card.title = "Controls";
    for (const auto& control : kControls) {
        if (!applies_to(control, frontend) || card.line_count >= card.lines.size())
            continue;
        char* out = lines[card.line_count];
        std::size_t n = 0;
        for (const auto part : {control.keys, std::string_view(": "), control.what})
            for (const char c : part)
                if (n + 1 < sizeof lines[0])
                    out[n++] = c;
        out[n] = 0;
        card.lines[card.line_count++] = std::string_view(out, n);
    }
    return card;
}
} // namespace globe
