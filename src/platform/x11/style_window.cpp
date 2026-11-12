// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "style_window.hpp"
#include "globe/strings.hpp"
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <cstring>
namespace globe {
namespace {
constexpr int kRowHeight = 30, kTop = 8;
constexpr int kWidth = StyleWindow::kWidth;
constexpr Str kNames[StyleWindow::kRows] = {Str::LookMap, Str::LookRelief};
// Swatches of what each look paints (land, sea), matching the palettes.
constexpr unsigned long kSwatch[StyleWindow::kRows][2] = {{0xB0C84E, 0x5696CD},
                                                          {0x8E9E53, 0x1E6AA8}};
} // namespace
StyleWindow::~StyleWindow() {
    if (gc_)
        XFreeGC(display_, gc_);
    // The window is a child of the Globe window and goes with it.
}
void StyleWindow::attach(Window owner, int x, int y, int current) {
    current_ = current;
    if (window_)
        return;
    const auto screen = DefaultScreen(display_);
    window_ = XCreateSimpleWindow(display_, owner, x, y, kWidth, kHeight, 0,
                                  BlackPixel(display_, screen), WhitePixel(display_, screen));
    XSelectInput(display_, window_, ExposureMask | ButtonPressMask);
    gc_ = XCreateGC(display_, window_, 0, nullptr);
    XStoreName(display_, window_, "Poodle Earth looks");
    XMapWindow(display_, window_);
}
void StyleWindow::place(int x, int y) {
    if (window_)
        XMoveWindow(display_, window_, x, y);
}
void StyleWindow::set_current(int row) {
    if (row == current_)
        return;
    current_ = row;
    if (window_)
        draw();
}
void StyleWindow::draw() {
    const auto screen = DefaultScreen(display_);
    XSetForeground(display_, gc_, WhitePixel(display_, screen));
    XFillRectangle(display_, window_, gc_, 0, 0, kWidth, kTop * 2 + kRowHeight * kRows);
    for (int i = 0; i < kRows; ++i) {
        const int y = kTop + i * kRowHeight;
        XSetForeground(display_, gc_, kSwatch[i][1]);
        XFillRectangle(display_, window_, gc_, 10, y + 4, 22, 22);
        XSetForeground(display_, gc_, kSwatch[i][0]);
        XFillRectangle(display_, window_, gc_, 10, y + 4, 22, 11);
        XSetForeground(display_, gc_, BlackPixel(display_, screen));
        XDrawRectangle(display_, window_, gc_, 10, y + 4, 22, 22);
        const char* name = tr(kNames[i]);
        XDrawString(display_, window_, gc_, 42, y + 20, name, static_cast<int>(std::strlen(name)));
        if (i == current_) {
            XDrawRectangle(display_, window_, gc_, 4, y, kWidth - 9, kRowHeight - 2);
            XDrawString(display_, window_, gc_, kWidth - 24, y + 20, "*", 1);
        }
    }
}
StyleWindow::Result StyleWindow::handle(const XEvent& e) {
    Result r;
    if (!window_ || e.xany.window != window_)
        return r;
    r.handled = true;
    if (e.type == Expose && e.xexpose.count == 0)
        draw();
    if (e.type == ButtonPress && e.xbutton.button == Button1) {
        const int row = (e.xbutton.y - kTop) / kRowHeight;
        if (e.xbutton.y >= kTop && row >= 0 && row < kRows)
            r.picked = row;
    }
    return r;
}
} // namespace globe
