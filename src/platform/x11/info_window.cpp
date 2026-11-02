// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "info_window.hpp"
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <algorithm>
#include <string>
namespace globe {
InfoWindow::~InfoWindow() {
    if (gc_)
        XFreeGC(display_, gc_);
    if (window_)
        XDestroyWindow(display_, window_);
}
void InfoWindow::show(Window owner, InfoCard card, int x) {
    card_ = card;
    card_.line_count = std::min(card_.line_count, static_cast<unsigned>(card_.lines.size()));
    offset_ = 0;
    if (!window_) {
        const auto screen = DefaultScreen(display_);
        window_ =
            XCreateSimpleWindow(display_, RootWindow(display_, screen), x, 0, width_, height_, 1,
                                BlackPixel(display_, screen), WhitePixel(display_, screen));
        XSetTransientForHint(display_, window_, owner);
        XSelectInput(display_, window_,
                     ExposureMask | StructureNotifyMask | ButtonPressMask | ButtonReleaseMask |
                         ButtonMotionMask | KeyPressMask);
        close_ = XInternAtom(display_, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(display_, window_, &close_, 1);
        XSizeHints hints{};
        hints.flags = PMinSize;
        hints.min_width = 160;
        hints.min_height = 100;
        XSetWMNormalHints(display_, window_, &hints);
        gc_ = XCreateGC(display_, window_, 0, nullptr);
    }
    XStoreName(display_, window_, "Globe information");
    XMapRaised(display_, window_);
    draw();
}
void InfoWindow::scroll(int lines) {
    const int visible = std::max(1, (height_ - 48) / 20);
    const int next =
        std::clamp(offset_ + lines, 0, std::max(0, static_cast<int>(card_.line_count) - visible));
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
    for (int i = 0; i < visible && offset_ + i < static_cast<int>(card_.line_count); ++i) {
        const auto line = card_.lines[static_cast<unsigned>(offset_ + i)];
        XDrawString(display_, window_, gc_, 8, 48 + 20 * i, line.data(),
                    static_cast<int>(line.size()));
    }
    XSetClipMask(display_, gc_, 0);
    const int track = height_ - 16;
    XDrawRectangle(display_, window_, gc_, width_ - 20, 8, 12, track);
    const int thumb =
        std::max(12, track * std::min(visible, std::max(1, static_cast<int>(card_.line_count))) /
                         std::max(1, static_cast<int>(card_.line_count)));
    const int max = std::max(0, static_cast<int>(card_.line_count) - visible);
    const int top = 8 + (max ? (track - thumb) * offset_ / max : 0);
    XFillRectangle(display_, window_, gc_, width_ - 18, top, 9, thumb);
}
bool InfoWindow::handle(const XEvent& e) {
    if (!window_ || e.xany.window != window_)
        return false;
    if (e.type == Expose && e.xexpose.count == 0)
        draw();
    if (e.type == ConfigureNotify) {
        width_ = std::clamp(e.xconfigure.width, 160, 32767);
        height_ = std::clamp(e.xconfigure.height, 100, 32767);
        scroll(0);
        draw();
    }
    if (e.type == ClientMessage && static_cast<Atom>(e.xclient.data.l[0]) == close_)
        XUnmapWindow(display_, window_);
    if (e.type == KeyPress) {
        const auto key = XLookupKeysym(const_cast<XKeyEvent*>(&e.xkey), 0);
        if (key == XK_Down || key == XK_Page_Down)
            scroll(key == XK_Down ? 1 : 5);
        if (key == XK_Up || key == XK_Page_Up)
            scroll(key == XK_Up ? -1 : -5);
        if (key == XK_Escape)
            XUnmapWindow(display_, window_);
    }
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
                      max = std::max(0, static_cast<int>(card_.line_count) - visible);
            scroll((std::max(0, y - 8) * max / std::max(1, height_ - 16)) - offset_);
        }
    }
    return true;
}
} // namespace globe
