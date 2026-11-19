// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Host-only prototype for #148: where the Globe, Information and Looks windows go, and how a window
// that has strayed is brought back. Pure integer geometry in global screen coordinates (y down), no
// Toolbox, no state. It is research code: nothing in the product includes it. If the workspace is
// built, this header (or its rules) moves to src/platform/common/ beside classic_window_fit.hpp.
#pragma once
#include <algorithm>

namespace globe::workspace {

struct Rect {
    int left, top, right, bottom;
    constexpr int width() const { return right - left; }
    constexpr int height() const { return bottom - top; }
    friend constexpr bool operator==(const Rect&, const Rect&) = default;
};

// The screen holding the menu bar (GetMainDevice) and the height of that bar (GetMBarHeight).
struct Screen {
    Rect bounds;
    int menu_bar = 20;
};

// Window furniture, as in src/platform/os9/main.cpp window_content(): a document window's title bar
// sits above its content rectangle, a one-pixel frame around the other three sides.
constexpr int kGap = 6, kTitleBar = 20, kFrame = 1;
// Content sizes. The Information size is the 300 x 200 starting point from the #148 assignment; the
// palette holds the two Looks rows of the sidebar (22 px each) and nothing else.
constexpr int kInfoW = 300, kInfoH = 200, kInfoMinW = 160, kInfoMinH = 80;
constexpr int kPaletteW = 100, kPaletteH = 48;
// Same floors as classic_window_fit.hpp.
constexpr int kMinGlobeW = 256, kMinGlobeH = 256;
// The globe pane the sidebar leaves at 640 x 480 (628 - 220). A workspace that opens beside windows
// must not give the globe less than the layout it replaces.
constexpr int kBaselineGlobeW = 408;
// The part of a title bar that must stay on screen so the window can always be grabbed.
constexpr int kGrabW = 64;
// The grow box limit used today (main.cpp): no globe larger than this.
constexpr int kMaxGlobeW = 1600, kMaxGlobeH = 1200;

// The window frame around a content rectangle.
constexpr Rect structure(const Rect& content) {
    return {content.left - kFrame, content.top - kTitleBar, content.right + kFrame,
            content.bottom + kFrame};
}
constexpr Rect title_bar(const Rect& content) {
    return {content.left - kFrame, content.top - kTitleBar, content.right + kFrame, content.top};
}
// The desktop below the menu bar, as a structure-rectangle area.
constexpr Rect desktop(const Screen& s) {
    return {s.bounds.left, s.bounds.top + s.menu_bar, s.bounds.right, s.bounds.bottom};
}

enum class Mode {
    Beside,  // the screen is wide enough for the globe and a column of windows
    Compact, // it is not: the globe takes the desktop and the other windows start closed
};

struct Arrangement {
    Mode mode;
    Rect globe;         // content rectangles
    Rect info, palette; // where each opens (its "home"), whether or not it starts open
    bool info_open, palette_open;
};

// The current single-window content rectangle (main.cpp window_content()) for a given width taken
// by whatever sits beside the globe.
constexpr Rect globe_content(const Screen& s, int beside_width) {
    const Rect d = desktop(s);
    const int top = d.top + kTitleBar + kGap;
    const int left = d.left + kGap;
    const int width = std::clamp(d.right - kGap - left - beside_width, kMinGlobeW, kMaxGlobeW);
    const int height = std::clamp(d.bottom - kGap - top, kMinGlobeH, kMaxGlobeH);
    return {left, top, left + width, top + height};
}

// Where the two auxiliary windows go when they sit in a right-hand column beside `globe`.
constexpr Rect column_palette(const Rect& globe) {
    return {globe.right + kGap, globe.top, globe.right + kGap + kPaletteW, globe.top + kPaletteH};
}
constexpr Rect column_info(const Rect& globe, int info_h = kInfoH) {
    const int top = globe.top + kPaletteH + kGap + kTitleBar;
    return {globe.right + kGap, top, globe.right + kGap + kInfoW, top + info_h};
}

// Moves a content rectangle so its structure lies inside the desktop, keeping its size when it can.
constexpr Rect fit_inside_desktop(Rect content, const Screen& s, int min_w, int min_h) {
    const Rect d = desktop(s);
    const int max_w = std::max(d.right - d.left - 2 * kFrame, min_w);
    const int max_h = std::max(d.bottom - d.top - kTitleBar - kFrame, min_h);
    const int w = std::clamp(content.width(), min_w, max_w),
              h = std::clamp(content.height(), min_h, max_h);
    const int left =
        std::clamp(content.left, d.left + kFrame, std::max(d.right - kFrame - w, d.left + kFrame));
    const int top = std::clamp(content.top, d.top + kTitleBar,
                               std::max(d.bottom - kFrame - h, d.top + kTitleBar));
    return {left, top, left + w, top + h};
}

// The arrangement at launch. Beside needs the globe to keep at least the pane the sidebar leaves it
// and the column to be tall enough for the palette over the Information window; otherwise Compact.
constexpr Arrangement initial_arrangement(const Screen& s) {
    const Rect compact = globe_content(s, 0);
    const Rect d = desktop(s);
    const int room = d.right - kGap - (d.left + kGap);
    const int column_h = kTitleBar + kPaletteH + kGap + kTitleBar + kInfoH;
    const bool beside = room - kGap - kInfoW >= kBaselineGlobeW && compact.height() >= column_h;
    if (beside) {
        const Rect globe = globe_content(s, kGap + kInfoW);
        return {Mode::Beside, globe, column_info(globe), column_palette(globe), true, true};
    }
    // Home positions overlap the globe: the lower right corner for Information, the upper right for
    // the palette. They are used only when the user asks for those windows.
    const Rect info{compact.right - kInfoW, compact.bottom - kInfoH, compact.right, compact.bottom};
    const Rect palette{compact.right - kPaletteW, compact.top, compact.right,
                       compact.top + kPaletteH};
    return {Mode::Compact,
            compact,
            fit_inside_desktop(info, s, kInfoMinW, kInfoMinH),
            fit_inside_desktop(palette, s, kPaletteW, kPaletteH),
            false,
            false};
}

// Is enough of the title bar on the desktop to grab it? At least kGrabW (or the whole bar if it is
// narrower) of its width must lie on the desktop, and all of its height below the menu bar.
constexpr bool grabbable(const Rect& content, const Screen& s) {
    const Rect bar = title_bar(content);
    const Rect d = desktop(s);
    if (bar.top < d.top || bar.bottom > d.bottom)
        return false;
    const int visible = std::min(bar.right, d.right) - std::max(bar.left, d.left);
    return visible >= std::min(kGrabW, bar.width());
}

// Brings a window back: any content rectangle in, one whose title bar is grabbable out. A window
// that is already grabbable is left alone, so recovery never moves a window the user placed
// sensibly. Used on Show (Window menu), after a screen-bounds change, and for Standard Positions.
constexpr Rect recover(const Rect& content, const Screen& s, int min_w, int min_h) {
    if (grabbable(content, s))
        return content;
    return fit_inside_desktop(content, s, min_w, min_h);
}

// What the Globe window is allowed to cover. Used to measure how much of the map an auxiliary
// window hides: the area of `a` inside `b`, in pixels.
constexpr long overlap_area(const Rect& a, const Rect& b) {
    const int w = std::min(a.right, b.right) - std::max(a.left, b.left);
    const int h = std::min(a.bottom, b.bottom) - std::max(a.top, b.top);
    return w > 0 && h > 0 ? static_cast<long>(w) * h : 0;
}

} // namespace globe::workspace
