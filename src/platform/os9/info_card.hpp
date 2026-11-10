// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// A small Toolbox window that shows an information card (a country, a place or a measured
// distance): a title and up to twelve lines, scrolled with a scroll bar when they do not fit.
// Showing a new card never activates the window; the workspace layout is the caller's.
#pragma once
#include "globe/countries.hpp"
#include <Windows.h>
namespace globe {
class InfoCardWindow {
  public:
    InfoCardWindow() = default;
    InfoCardWindow(const InfoCardWindow&) = delete;
    InfoCardWindow& operator=(const InfoCardWindow&) = delete;
    ~InfoCardWindow();
    // Creates the window with `content` (global coordinates), if it does not exist yet, and shows
    // `card` in it. It never sizes, moves or activates an existing window.
    void show(const InfoCard& card, const Rect& content);
    WindowPtr window() const { return window_; }
    // Redraws the contents; call inside BeginUpdate/EndUpdate or right after showing.
    void draw();
    // A mouse press on this window (inGoAway/inDrag/inContent); returns true when handled.
    bool mouse_down(const EventRecord& event);
    // The window became active or inactive: the scroll bar dims with it.
    void activate(bool active);

  private:
    unsigned visible_lines() const;
    void sync_scroll();
    WindowPtr window_ = nullptr;
    ControlHandle scroll_ = nullptr;
    InfoCard card_{};
    unsigned offset_ = 0; // first line shown
};
} // namespace globe
