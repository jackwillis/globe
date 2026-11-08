// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "looks_palette.hpp"
#include <Events.h>
#include <Quickdraw.h>
#include <algorithm>
namespace globe {
namespace {
constexpr short kWidth = 120, kRow = 22, kMargin = 6;
constexpr const char* kNames[kLooksRows] = {"Map", "Relief"};
constexpr unsigned char kCheckMark = 0x12; // the check in the system font
} // namespace
LooksPalette::~LooksPalette() {
    if (window_)
        DisposeWindow(window_);
}
void LooksPalette::show(TerrainStyle current, short main_right) {
    shown_ = current;
    if (!window_) {
        const short height = kRow * kLooksRows;
        const short left = static_cast<short>(
            std::min<int>(main_right + 12, qd.screenBits.bounds.right - kWidth - 4));
        Rect bounds{300, left, static_cast<short>(300 + height), static_cast<short>(left + kWidth)};
        const unsigned char title[] = {5, 'L', 'o', 'o', 'k', 's'};
        window_ = NewCWindow(nullptr, &bounds, title, true, noGrowDocProc,
                             reinterpret_cast<WindowPtr>(-1), true, 0);
        if (!window_)
            return;
    } else {
        ShowWindow(window_);
        SelectWindow(window_);
    }
    visible_ = true;
    SetPortWindowPort(window_);
    InvalRect(&window_->portRect);
    draw();
}
void LooksPalette::sync(TerrainStyle current) {
    if (current == shown_)
        return;
    shown_ = current;
    if (window_ && visible_) {
        SetPortWindowPort(window_);
        draw();
    }
}
void LooksPalette::draw() {
    if (!window_)
        return;
    SetPortWindowPort(window_);
    EraseRect(&window_->portRect);
    TextFont(0);
    TextSize(12);
    for (int row = 0; row < kLooksRows; ++row) {
        const bool active = looks_style(row) == shown_;
        TextFace(active ? bold : normal);
        if (active) {
            MoveTo(kMargin, static_cast<short>(row * kRow + 15));
            DrawChar(static_cast<char>(kCheckMark));
        }
        MoveTo(static_cast<short>(kMargin + 16), static_cast<short>(row * kRow + 15));
        const char* name = kNames[row];
        unsigned char text[16];
        unsigned char n = 0;
        while (name[n] && n < 15) {
            text[n + 1] = static_cast<unsigned char>(name[n]);
            ++n;
        }
        text[0] = n;
        DrawString(text);
    }
    TextFace(normal);
}
bool LooksPalette::mouse_down(const EventRecord& event, int& row) {
    row = -1;
    if (!window_)
        return false;
    WindowPtr hit = nullptr;
    const auto part = FindWindow(event.where, &hit);
    if (hit != window_)
        return false;
    if (part == inGoAway) {
        if (TrackGoAway(window_, event.where)) {
            HideWindow(window_);
            visible_ = false;
        }
    } else if (part == inDrag) {
        Rect limit{20, -32000, 32000, 32000};
        DragWindow(window_, event.where, &limit);
    } else if (part == inContent) {
        if (FrontWindow() != window_) {
            SelectWindow(window_); // the first click only activates, as for a document window
        } else {
            SetPortWindowPort(window_);
            Point local = event.where;
            GlobalToLocal(&local);
            row = local.v >= 0 ? local.v / kRow : -1;
        }
    }
    return true;
}
} // namespace globe
