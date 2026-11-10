// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include "globe/prefs.hpp"
#include <X11/Xlib.h>
namespace globe {
// A small palette window listing the looks, one row each, with the current one marked. It only
// reports which row was clicked; the application turns that into a model message.
class StyleWindow {
  public:
    static constexpr int kRows = 2;
    struct Result {
        bool handled = false;
        int picked = -1; // row clicked, or -1
    };
    explicit StyleWindow(Display* display) : display_(display) {}
    ~StyleWindow();
    static constexpr int kWidth = 220, kHeight = 76;
    // A child of `owner` (the one Globe window) at (x, y).
    void attach(Window owner, int x, int y, int current);
    void place(int x, int y);
    void set_current(int row);
    void set_language(Language language);
    Result handle(const XEvent& event);

  private:
    void draw();
    Display* display_;
    Window window_ = 0;
    GC gc_ = nullptr;
    int current_ = 0;
    Language language_ = Language::English;
};
} // namespace globe
