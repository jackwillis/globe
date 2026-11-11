// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// The About window: a plain, non-modal document window (no grow box) with the program's name,
// licence and data pointers. It is created on first use and hidden, not destroyed, when closed.
#pragma once
#include "globe/prefs.hpp"
#include <Windows.h>
namespace globe {
class AboutWindow {
  public:
    AboutWindow() = default;
    AboutWindow(const AboutWindow&) = delete;
    AboutWindow& operator=(const AboutWindow&) = delete;
    ~AboutWindow();
    // Shows the window in front, creating it the first time.
    bool open();
    void close();
    WindowPtr window() const { return window_; }
    // Draws the contents into the window's port; call inside BeginUpdate/EndUpdate or after open().
    void draw();

  private:
    WindowPtr window_ = nullptr;
    bool shown_flag_ = false; // the window is on screen
};
} // namespace globe
