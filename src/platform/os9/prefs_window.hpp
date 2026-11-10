// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// The Preferences window: a plain non-modal document window with radio rows for the units and the
// language. It holds no preference of its own: it draws the Model's and reports a click as the
// preferences the user asked for, which the shell sends through update().
#pragma once
#include "globe/prefs.hpp"
#include <Windows.h>
#include <optional>
namespace globe {
class PrefsWindow {
  public:
    PrefsWindow() = default;
    PrefsWindow(const PrefsWindow&) = delete;
    PrefsWindow& operator=(const PrefsWindow&) = delete;
    ~PrefsWindow();
    bool open(const Preferences& current);
    void close();
    WindowPtr window() const { return window_; }
    // The Model's preferences changed (maybe from elsewhere): retitle and redraw if shown.
    void sync(const Preferences& current);
    void draw();
    // A press at window-local `local`: the preferences it selects, if a different row was hit.
    std::optional<Preferences> mouse_down(Point local) const;

  private:
    WindowPtr window_ = nullptr;
    bool shown_flag_ = false; // the window is on screen
    Preferences shown_{};
};
} // namespace globe
