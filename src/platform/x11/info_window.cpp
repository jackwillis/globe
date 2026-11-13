// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "info_window.hpp"
#include "globe/wrap.hpp"
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <algorithm>
#include <string>
namespace globe {
InfoWindow::~InfoWindow() {
    if (gc_)
        XFreeGC(display_, gc_);
    // The window is a child of the Globe window and goes with it.
}
void InfoWindow::attach(Window owner, int x, int y, int width, int height) {
    if (window_)
        return;
    const auto screen = DefaultScreen(display_);
    width_ = std::max(1, width);
    height_ = std::max(1, height);
    window_ = XCreateSimpleWindow(display_, owner, x, y, static_cast<unsigned>(width_),
                                  static_cast<unsigned>(height_), 0, BlackPixel(display_, screen),
                                  WhitePixel(display_, screen));
    // No key events: keys pressed over the card reach the Globe window and its shortcuts.
    XSelectInput(display_, window_,
                 ExposureMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);
    gc_ = XCreateGC(display_, window_, 0, nullptr);
    XStoreName(display_, window_, "Poodle Earth information");
    XMapWindow(display_, window_);
}
void InfoWindow::place(int x, int y, int width, int height) {
    if (!window_)
        return;
    width_ = std::max(1, width);
    height_ = std::max(1, height);
    XMoveResizeWindow(display_, window_, x, y, static_cast<unsigned>(width_),
                      static_cast<unsigned>(height_));
    rewrap();
    scroll(0);
    draw();
}
void InfoWindow::show(InfoCard card) {
    card_ = card;
    card_.line_count = std::min(card_.line_count, static_cast<unsigned>(card_.lines.size()));
    offset_ = 0;
    if (!window_)
        return;
    rewrap();
    draw();
}
// Breaks the card's lines to the width left of the scroll bar, in the GC's font.
void InfoWindow::rewrap() {
    XFontStruct* font = XQueryFont(display_, XGContextFromGC(gc_));
    if (!font)
        return;
    const int room = std::max(1, width_ - 28 - 8);
    wrapped_ = wrap_card(card_, room, [&](std::string_view text) {
        return XTextWidth(font, text.data(), static_cast<int>(text.size()));
    });
    XFreeFontInfo(nullptr, font, 1);
}
void InfoWindow::scroll(int lines) {
    const int visible = std::max(1, (height_ - 48) / 20);
    const int next =
        std::clamp(offset_ + lines, 0, std::max(0, static_cast<int>(wrapped_.count) - visible));
    if (next != offset_) {
        offset_ = next;
        ++scrolls_;
        draw();
    }
}
void InfoWindow::draw() {
    const auto screen = DefaultScreen(display_);
    XSetForeground(display_, gc_, WhitePixel(display_, screen));
    XFillRectangle(display_, window_, gc_, 0, 0, width_, height_);
    XSetForeground(display_, gc_, BlackPixel(display_, screen));
    // Draw text inside a clip rectangle so long source titles never cover the scrollbar.
    XRectangle clip{4, 0, static_cast<unsigned short>(std::max(1, width_ - 28)),
                    static_cast<unsigned short>(height_)};
    XSetClipRectangles(display_, gc_, 0, 0, &clip, 1, Unsorted);
    XDrawString(display_, window_, gc_, 8, 22, card_.title.data(),
                static_cast<int>(card_.title.size()));
    const int visible = std::max(1, (height_ - 48) / 20);
    for (int i = 0; i < visible && offset_ + i < static_cast<int>(wrapped_.count); ++i) {
        const auto line = wrapped_.rows[static_cast<unsigned>(offset_ + i)];
        XDrawString(display_, window_, gc_, 8, 48 + 20 * i, line.data(),
                    static_cast<int>(line.size()));
    }
    XSetClipMask(display_, gc_, 0);
    const int track = height_ - 16;
    XDrawRectangle(display_, window_, gc_, width_ - 20, 8, 12, track);
    const int thumb =
        std::max(12, track * std::min(visible, std::max(1, static_cast<int>(wrapped_.count))) /
                         std::max(1, static_cast<int>(wrapped_.count)));
    const int max = std::max(0, static_cast<int>(wrapped_.count) - visible);
    const int top = 8 + (max ? (track - thumb) * offset_ / max : 0);
    XFillRectangle(display_, window_, gc_, width_ - 18, top, 9, thumb);
}
bool InfoWindow::handle(const XEvent& e) {
    if (!window_ || e.xany.window != window_)
        return false;
    if (e.type == Expose && e.xexpose.count == 0)
        draw();
    if (e.type == ButtonPress) {
        if (e.xbutton.button == Button4)
            scroll(-1);
        if (e.xbutton.button == Button5)
            scroll(1);
    }
    if ((e.type == ButtonPress && e.xbutton.button == Button1) ||
        (e.type == MotionNotify && (e.xmotion.state & Button1Mask))) {
        const int x = e.type == ButtonPress ? e.xbutton.x : e.xmotion.x;
        const int y = e.type == ButtonPress ? e.xbutton.y : e.xmotion.y;
        if (x >= width_ - 24) {
            const int visible = std::max(1, (height_ - 48) / 20),
                      max = std::max(0, static_cast<int>(wrapped_.count) - visible);
            scroll((std::max(0, y - 8) * max / std::max(1, height_ - 16)) - offset_);
        }
    }
    return true;
}
} // namespace globe
