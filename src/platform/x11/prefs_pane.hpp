// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include "globe/prefs.hpp"
#include <X11/Xlib.h>
#include <optional>
namespace globe {
// Unit radio rows, a child of the one Globe window under the looks list. It
// holds no preference of its own: it draws the Model's and reports a click as the preferences
// asked for, which the application sends through update().
class PrefsPane {
  public:
    static constexpr int kWidth = 220, kHeight = 66;
    struct Result {
        bool handled = false;
        std::optional<Preferences> next;
    };
    explicit PrefsPane(Display* display) : display_(display) {}
    ~PrefsPane();
    void attach(Window owner, int x, int y, const Preferences& current);
    void place(int x, int y);
    void sync(const Preferences& current);
    Result handle(const XEvent& event);

  private:
    void draw();
    Display* display_;
    Window window_ = 0;
    GC gc_ = nullptr;
    Preferences shown_{};
};
} // namespace globe
