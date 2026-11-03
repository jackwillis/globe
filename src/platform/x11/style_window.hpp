// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
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
    void show(Window owner, int x, int current);
    void set_current(int row);
    Result handle(const XEvent& event);

  private:
    void draw();
    Display* display_;
    Window window_ = 0;
    GC gc_ = nullptr;
    Atom close_ = 0;
    int current_ = 0;
};
} // namespace globe
