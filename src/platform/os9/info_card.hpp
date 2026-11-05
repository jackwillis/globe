// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// A small Toolbox window that shows an information card (a country, a place or a measured
// distance): a title and up to twelve lines, closed with its close box.
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
    // Shows (or replaces) the card to the right of `main_right` (global coordinates).
    void show(const InfoCard& card, short main_right);
    WindowPtr window() const { return window_; }
    // Redraws the contents; call inside BeginUpdate/EndUpdate or right after showing.
    void draw();
    // A mouse press on this window (inGoAway/inDrag/inContent); returns true when handled.
    bool mouse_down(const EventRecord& event);

  private:
    WindowPtr window_ = nullptr;
    InfoCard card_{};
};
} // namespace globe
