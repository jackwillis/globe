// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// The looks palette: a small Toolbox window with one row per look (Map, Relief), a check mark on
// the look in use. It holds no state of its own beyond what it last drew; the look is Model's.
#pragma once
#include "globe/looks.hpp"
#include <Windows.h>
namespace globe {
class LooksPalette {
  public:
    LooksPalette() = default;
    LooksPalette(const LooksPalette&) = delete;
    LooksPalette& operator=(const LooksPalette&) = delete;
    ~LooksPalette();
    // Shows the palette (creating it on first use) to the right of `main_right`, or brings it up.
    void show(TerrainStyle current, short main_right);
    WindowPtr window() const { return window_; }
    bool visible() const { return visible_; }
    // The look changed elsewhere (a key, the toolbar): redraw the check mark if it is showing.
    void sync(TerrainStyle current);
    // Redraws the contents; call inside BeginUpdate/EndUpdate or right after showing.
    void draw();
    // A mouse press on this window. Returns the row clicked in the content (-1 for none) through
    // `row`; the close box, drag bar and activation are handled here. Returns true when handled.
    bool mouse_down(const EventRecord& event, int& row);

  private:
    WindowPtr window_ = nullptr;
    bool visible_ = false;
    TerrainStyle shown_ = TerrainStyle::Map;
};
} // namespace globe
