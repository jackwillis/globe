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
    void show(Window owner, InfoCard card, int x);
    void move(int x) {
        if (window_)
            XMoveWindow(display_, window_, x, 0);
    }
    bool handle(const XEvent& event);
    std::size_t scrolls() const { return scrolls_; }

  private:
    void draw();
    void rewrap();
    void scroll(int lines);
    Display* display_;
    Window window_ = 0;
    GC gc_ = nullptr;
    Atom close_ = 0;
    InfoCard card_{};
    WrappedCard wrapped_{};
    int width_ = 360, height_ = 150, offset_ = 0;
    std::size_t scrolls_ = 0;
};
} // namespace globe
