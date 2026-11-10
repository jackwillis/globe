// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "sidebar.hpp"
#include <Events.h>
#include <Quickdraw.h>
#include "globe/wrap.hpp"
#include <algorithm>
#include <string_view>
namespace globe {
namespace {
constexpr short kRow = 22, kLine = 14, kHead = 22, kMargin = 8, kBar = 16, kGrow = 15;
constexpr const char* kNames[kLooksRows] = {"Map", "Relief"};
constexpr unsigned char kCheckMark = 0x12; // the check in the system font
// Pascal string (length byte first) from text, truncated to fit one line.
void to_pascal(unsigned char* out, std::string_view text) {
    const auto n = static_cast<unsigned char>(std::min<std::size_t>(text.size(), 60));
    out[0] = n;
    for (unsigned char i = 0; i < n; ++i)
        out[i + 1] = static_cast<unsigned char>(text[i]);
}
} // namespace
bool Sidebar::attach(WindowPtr window) {
    window_ = window;
    Rect bar{0, 0, 1, 1};
    const unsigned char no_title[] = {0};
    scroll_ = NewControl(window_, &bar, no_title, true, 0, 0, 0, scrollBarProc, 0);
    return scroll_ != nullptr;
}
unsigned Sidebar::visible_lines() const {
    const int rows = (card_area_.bottom - card_area_.top - kHead - 4) / kLine;
    return static_cast<unsigned>(std::max(rows, 1));
}
// Keeps the scroll bar's range and the first line shown inside what the card needs.
void Sidebar::sync_scroll() {
    const unsigned visible = visible_lines();
    const unsigned most = wrapped_.count > visible ? wrapped_.count - visible : 0;
    offset_ = std::min(offset_, most);
    if (!scroll_)
        return;
    SetControlMaximum(scroll_, static_cast<short>(most));
    SetControlValue(scroll_, static_cast<short>(offset_));
    HiliteControl(scroll_, (most == 0 || !active_) ? 255 : 0);
}
void Sidebar::layout(short width, short height) {
    area_ = Rect{0, static_cast<short>(width - kWidth), height, width};
    card_area_ = Rect{static_cast<short>(kRow * kLooksRows + 1), area_.left, height, width};
    if (scroll_) {
        // The scroll bar takes the card's right edge, above the window's grow box.
        const short bottom = static_cast<short>(height - kGrow);
        MoveControl(scroll_, static_cast<short>(width - kBar + 1), card_area_.top);
        SizeControl(scroll_, kBar, std::max<short>(bottom - card_area_.top, 16));
    }
    rewrap();
    sync_scroll();
}
// Breaks the card's lines to the width between the margin and the scroll bar, in the drawing font.
void Sidebar::rewrap() {
    if (!window_)
        return;
    SetPortWindowPort(window_);
    TextFont(0);
    TextSize(12);
    TextFace(normal);
    const int room = (area_.right - area_.left) - kMargin - kBar - 4;
    wrapped_ = wrap_card(card_, room, [](std::string_view text) {
        return static_cast<int>(TextWidth(const_cast<char*>(text.data()), 0, static_cast<short>(text.size())));
    });
}
void Sidebar::show(const InfoCard& card) {
    card_ = card;
    card_.line_count = std::min(card_.line_count, static_cast<unsigned>(card_.lines.size()));
    offset_ = 0;
    rewrap();
    sync_scroll();
    draw();
}
void Sidebar::sync(TerrainStyle current) {
    if (current == shown_)
        return;
    shown_ = current;
    if (window_) {
        SetPortWindowPort(window_);
        ClipRect(&area_);
        draw_rows();
        ClipRect(&window_->portRect);
    }
}
void Sidebar::activate(bool active) {
    active_ = active;
    sync_scroll();
}
void Sidebar::draw_rows() {
    Rect rows{0, static_cast<short>(area_.left + 1), static_cast<short>(kRow * kLooksRows),
              area_.right};
    EraseRect(&rows);
    TextFont(0);
    TextSize(12);
    for (int row = 0; row < kLooksRows; ++row) {
        const bool current = looks_style(row) == shown_;
        TextFace(current ? bold : normal);
        if (current) {
            MoveTo(static_cast<short>(area_.left + kMargin), static_cast<short>(row * kRow + 15));
            DrawChar(static_cast<char>(kCheckMark));
        }
        MoveTo(static_cast<short>(area_.left + kMargin + 16), static_cast<short>(row * kRow + 15));
        unsigned char text[16];
        text[0] = 0;
        for (const char* name = kNames[row]; *name && text[0] < 15; ++name)
            text[++text[0]] = static_cast<unsigned char>(*name);
        DrawString(text);
    }
    TextFace(normal);
    MoveTo(area_.left, static_cast<short>(kRow * kLooksRows));
    LineTo(area_.right, static_cast<short>(kRow * kLooksRows));
}
void Sidebar::draw_card() {
    Rect body{static_cast<short>(card_area_.top + 1), static_cast<short>(card_area_.left + 1),
              card_area_.bottom, static_cast<short>(card_area_.right - kBar)};
    EraseRect(&body);
    TextFont(0);
    TextSize(12);
    unsigned char line[64];
    TextFace(bold);
    to_pascal(line, card_.title);
    MoveTo(static_cast<short>(area_.left + kMargin), static_cast<short>(card_area_.top + 16));
    DrawString(line);
    TextFace(normal);
    const unsigned last = std::min(wrapped_.count, offset_ + visible_lines());
    for (unsigned i = offset_; i < last; ++i) {
        to_pascal(line, wrapped_.rows[i]);
        MoveTo(static_cast<short>(area_.left + kMargin),
               static_cast<short>(card_area_.top + kHead +
                                  kLine * (static_cast<short>(i - offset_) + 1) - 2));
        DrawString(line);
    }
}
void Sidebar::draw() {
    if (!window_)
        return;
    SetPortWindowPort(window_);
    ClipRect(&area_);
    MoveTo(area_.left, 0); // the separator between the globe and the sidebar
    LineTo(area_.left, area_.bottom);
    draw_rows();
    draw_card();
    DrawControls(window_);
    ClipRect(&window_->portRect);
    DrawGrowIcon(window_);
}
int Sidebar::mouse_down(Point local) {
    if (!window_ || !PtInRect(local, &area_))
        return -1;
    if (local.v < kRow * kLooksRows)
        return local.v >= 0 ? local.v / kRow : -1;
    ControlHandle control = nullptr;
    if (FindControl(local, window_, &control) != 0 && control == scroll_) {
        const unsigned visible = visible_lines();
        int next = GetControlValue(scroll_);
        // Without an action proc TrackControl returns after one press, so each click moves once.
        const auto tracked = TrackControl(scroll_, local, nullptr);
        if (tracked == inUpButton)
            next -= 1;
        else if (tracked == inDownButton)
            next += 1;
        else if (tracked == inPageUp)
            next -= static_cast<int>(visible);
        else if (tracked == inPageDown)
            next += static_cast<int>(visible);
        const unsigned most = wrapped_.count > visible ? wrapped_.count - visible : 0;
        offset_ = static_cast<unsigned>(std::clamp(next, 0, static_cast<int>(most)));
        sync_scroll();
        draw();
    }
    return -1;
}
} // namespace globe
