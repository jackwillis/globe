// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include "globe/countries.hpp"
#include "globe/wrap.hpp"
#include <X11/Xlib.h>
namespace globe {
class InfoWindow {
  public:
    explicit InfoWindow(Display* display) : display_(display) {}
    ~InfoWindow();
    // A child of `owner` (the one Globe window), at (x, y), `width` x `height`.
    void attach(Window owner, int x, int y, int width, int height);
    // Moves and resizes it with the main window; rewraps the text.
    void place(int x, int y, int width, int height);
    void show(InfoCard card);
    bool handle(const XEvent& event);
    std::size_t scrolls() const { return scrolls_; }

  private:
    void draw();
    void rewrap();
    void scroll(int lines);
    Display* display_;
    Window window_ = 0;
    GC gc_ = nullptr;
    InfoCard card_{};
    WrappedCard wrapped_{};
    int width_ = 220, height_ = 150, offset_ = 0;
    std::size_t scrolls_ = 0;
};
} // namespace globe
