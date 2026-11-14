// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// The one list of the program's controls: every shortcut key, gesture and pointer action, with the
// words Help shows for it. `binding_for_char` (the shortcut keys both front ends honor) is read
// from this table, so Help cannot list a key that does nothing or miss one that does. Rows meant
// for one front end say so: the Classic Mac OS and X11 differ in the wheel, quitting and the
// toolbar, and Help shows only what the running front end really does.
#pragma once
#include "globe/app.hpp"
#include "globe/countries.hpp"
#include <cstdint>
#include <optional>
#include <string_view>
namespace globe {
enum class Frontend : std::uint8_t { ClassicMac, X11 };
struct Control {
    bool classic_mac, x11;             // the front ends the row applies to
    std::string_view keys;             // what the person presses or does, as Help prints it
    std::string_view what;             // what it does
    std::string_view chars;            // typed characters bound to it (any case); empty if none
    std::optional<Command> command;    // sent through update()
    KeyBinding::Overlay overlay;       // or an overlay or window the front end handles itself
    std::string_view classic_commands; // effective Command keys, including aliases
    std::string_view classic_keys;     // optional Classic wording for the plain form
};
// The whole table, in the order Help lists it.
const Control* controls();
unsigned control_count();
bool applies_to(const Control& control, Frontend frontend);
// Starting a measurement replaces the Help card (the measurement's own cards use the pane next);
// leaving it, and the other overlays, leave Help as it is.
constexpr bool measure_replaces_help(KeyBinding::Overlay which, bool measuring_after) {
    return which == KeyBinding::Overlay::Measure && measuring_after;
}
// Help as an information card for `frontend`: the title and one line per applicable control,
// "keys: what". Text is static, so the card may be kept as long as wanted.
InfoCard help_card(Frontend frontend);
} // namespace globe
