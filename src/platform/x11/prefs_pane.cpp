// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "prefs_pane.hpp"
#include "globe/strings.hpp"
#include <cstring>
namespace globe {
namespace {
constexpr int kUnitsHead = 16, kUnitsTop = 22, kRow = 18;
void text(Display* d, Window w, GC gc, int x, int y, const char* s) {
    XDrawString(d, w, gc, x, y, s, static_cast<int>(std::strlen(s)));
}
void radio(Display* d, Window w, GC gc, int top, bool on, const char* label) {
    XDrawArc(d, w, gc, 10, top + 3, 11, 11, 0, 360 * 64);
    if (on)
        XFillArc(d, w, gc, 13, top + 6, 6, 6, 0, 360 * 64);
    text(d, w, gc, 30, top + 13, label);
}
} // namespace
PrefsPane::~PrefsPane() {
    if (gc_)
        XFreeGC(display_, gc_);
}
void PrefsPane::attach(Window owner, int x, int y, const Preferences& current) {
    shown_ = current;
    if (window_)
        return;
    const auto screen = DefaultScreen(display_);
    window_ = XCreateSimpleWindow(display_, owner, x, y, kWidth, kHeight, 0,
                                  BlackPixel(display_, screen), WhitePixel(display_, screen));
    XSelectInput(display_, window_, ExposureMask | ButtonPressMask);
    gc_ = XCreateGC(display_, window_, 0, nullptr);
    XStoreName(display_, window_, "Globe preferences");
    XMapWindow(display_, window_);
}
void PrefsPane::place(int x, int y) {
    if (window_)
        XMoveWindow(display_, window_, x, y);
}
void PrefsPane::sync(const Preferences& current) {
    if (current == shown_)
        return;
    shown_ = current;
    if (window_)
        draw();
}
void PrefsPane::draw() {
    const auto screen = DefaultScreen(display_);
    XSetForeground(display_, gc_, WhitePixel(display_, screen));
    XFillRectangle(display_, window_, gc_, 0, 0, kWidth, kHeight);
    XSetForeground(display_, gc_, BlackPixel(display_, screen));
    XDrawLine(display_, window_, gc_, 0, 0, kWidth, 0);
    text(display_, window_, gc_, 10, kUnitsHead, tr(Str::PrefsUnits));
    radio(display_, window_, gc_, kUnitsTop, shown_.units == Units::Metric, tr(Str::UnitsMetric));
    radio(display_, window_, gc_, kUnitsTop + kRow, shown_.units == Units::Imperial,
          tr(Str::UnitsImperial));
}
PrefsPane::Result PrefsPane::handle(const XEvent& e) {
    Result r;
    if (!window_ || e.xany.window != window_)
        return r;
    r.handled = true;
    if (e.type == Expose && e.xexpose.count == 0)
        draw();
    if (e.type == ButtonPress && e.xbutton.button == Button1) {
        const int y = e.xbutton.y;
        Preferences next = shown_;
        const auto row = [&](int top) { return y >= top && y < top + kRow; };
        if (row(kUnitsTop))
            next.units = Units::Metric;
        else if (row(kUnitsTop + kRow))
            next.units = Units::Imperial;
        if (!(next == shown_))
            r.next = next;
    }
    return r;
}
} // namespace globe
