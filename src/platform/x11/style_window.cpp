// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "style_window.hpp"
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <cstring>
namespace globe {
namespace {
constexpr int kWidth = 170, kRowHeight = 30, kTop = 8;
const char* const kNames[StyleWindow::kRows] = {"Map", "Relief"};
// Swatches of what each look paints (land, sea), matching the palettes.
constexpr unsigned long kSwatch[StyleWindow::kRows][2] = {{0xB0C84E, 0x5696CD},
                                                          {0x8E9E53, 0x1E6AA8}};
} // namespace
StyleWindow::~StyleWindow() {
    if (gc_)
        XFreeGC(display_, gc_);
    if (window_)
        XDestroyWindow(display_, window_);
}
void StyleWindow::show(Window owner, int x, int current) {
    current_ = current;
    if (!window_) {
        const auto screen = DefaultScreen(display_);
        window_ = XCreateSimpleWindow(display_, RootWindow(display_, screen), x, 200, kWidth,
                                      kTop * 2 + kRowHeight * kRows, 1,
                                      BlackPixel(display_, screen), WhitePixel(display_, screen));
        XSetTransientForHint(display_, window_, owner);
        XSelectInput(display_, window_, ExposureMask | ButtonPressMask | KeyPressMask);
        close_ = XInternAtom(display_, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(display_, window_, &close_, 1);
        XSizeHints hints{};
        hints.flags = PMinSize | PMaxSize;
        hints.min_width = hints.max_width = kWidth;
        hints.min_height = hints.max_height = kTop * 2 + kRowHeight * kRows;
        XSetWMNormalHints(display_, window_, &hints);
        gc_ = XCreateGC(display_, window_, 0, nullptr);
        XStoreName(display_, window_, "Globe looks");
    }
    XMapRaised(display_, window_);
    draw();
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
        XDrawString(display_, window_, gc_, 42, y + 20, kNames[i],
                    static_cast<int>(std::strlen(kNames[i])));
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
    if (e.type == ClientMessage && static_cast<Atom>(e.xclient.data.l[0]) == close_)
        XUnmapWindow(display_, window_);
    if (e.type == KeyPress && XLookupKeysym(const_cast<XKeyEvent*>(&e.xkey), 0) == XK_Escape)
        XUnmapWindow(display_, window_);
    if (e.type == ButtonPress && e.xbutton.button == Button1) {
        const int row = (e.xbutton.y - kTop) / kRowHeight;
        if (e.xbutton.y >= kTop && row >= 0 && row < kRows)
            r.picked = row;
    }
    return r;
}
} // namespace globe
