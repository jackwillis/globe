// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// The sidebar of the single Globe window: the looks list (Map, Relief, a check on the look in use)
// over the information card (a country, a place or a measured distance), which scrolls when its
// lines do not fit. It is drawn into the window's own port on the right of the globe pane, so it
// has no window, activation or placement of its own. It holds no state beyond what it last drew;
// the look is Model's.
#pragma once
#include "globe/countries.hpp"
#include "globe/looks.hpp"
#include <Windows.h>
namespace globe {
class Sidebar {
  public:
    static constexpr short kWidth = 220;
    Sidebar() = default;
    Sidebar(const Sidebar&) = delete;
    Sidebar& operator=(const Sidebar&) = delete;
    // Creates the scroll bar in `window` (the window's port must be current).
    bool attach(WindowPtr window);
    // The window content is now `width` x `height`: the sidebar takes the right `kWidth` columns.
    void layout(short width, short height);
    // The left edge of the sidebar in window-local coordinates (the globe pane is to its left).
    short left() const { return area_.left; }
    bool contains(Point local) const { return PtInRect(local, &area_); }
    // Shows a card, scrolled to the top; call draw() or let the next update do it.
    void show(const InfoCard& card);
    // The look changed elsewhere (a key, a menu): redraw the check mark.
    void sync(TerrainStyle current);
    // The window became active or inactive: the scroll bar dims with it.
    void activate(bool active);
    // Draws the whole sidebar; clipped to its area, so safe to call at any time or in an update.
    void draw();
    // A mouse press at `local` inside the sidebar. Returns the look row clicked (-1 for none).
    int mouse_down(Point local);

  private:
    unsigned visible_lines() const;
    void sync_scroll();
    void draw_rows();
    void draw_card();
    WindowPtr window_ = nullptr;
    ControlHandle scroll_ = nullptr;
    Rect area_{0, 0, 0, 0};
    Rect card_area_{0, 0, 0, 0};
    InfoCard card_{};
    TerrainStyle shown_ = TerrainStyle::Map;
    unsigned offset_ = 0; // first card line shown
    bool active_ = true;
};
} // namespace globe
