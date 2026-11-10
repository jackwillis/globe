// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "info_card.hpp"
#include <Events.h>
#include <Quickdraw.h>
#include <algorithm>
namespace globe {
namespace {
constexpr short kLine = 14, kTop = 22, kMargin = 8, kBar = 16;
// Pascal string (length byte first) from text, truncated to fit one line.
void to_pascal(unsigned char* out, std::string_view text) {
    const auto n = static_cast<unsigned char>(std::min<std::size_t>(text.size(), 60));
    out[0] = n;
    for (unsigned char i = 0; i < n; ++i)
        out[i + 1] = static_cast<unsigned char>(text[i]);
}
} // namespace
InfoCardWindow::~InfoCardWindow() {
    if (window_)
        DisposeWindow(window_);
}
unsigned InfoCardWindow::visible_lines() const {
    if (!window_)
        return 0;
    const int rows = (window_->portRect.bottom - window_->portRect.top - kTop - 4) / kLine;
    return static_cast<unsigned>(std::max(rows, 1));
}
// Keeps the scroll bar's range and the first line shown inside what the card needs.
void InfoCardWindow::sync_scroll() {
    const unsigned visible = visible_lines();
    const unsigned most = card_.line_count > visible ? card_.line_count - visible : 0;
    offset_ = std::min(offset_, most);
    if (!scroll_)
        return;
    SetControlMaximum(scroll_, static_cast<short>(most));
    SetControlValue(scroll_, static_cast<short>(offset_));
    HiliteControl(scroll_, most == 0 ? 255 : 0);
}
void InfoCardWindow::show(const InfoCard& card, const Rect& content) {
    card_ = card;
    card_.line_count = std::min(card_.line_count, static_cast<unsigned>(card_.lines.size()));
    offset_ = 0;
    if (!window_) {
        Rect bounds = content;
        unsigned char title[20];
        to_pascal(title, "Globe information");
        window_ = NewCWindow(nullptr, &bounds, title, true, noGrowDocProc,
                             reinterpret_cast<WindowPtr>(-1), true, 0);
        if (!window_)
            return;
        SetPortWindowPort(window_);
        const Rect port = window_->portRect;
        Rect bar{static_cast<short>(port.top - 1), static_cast<short>(port.right - kBar + 1),
                 static_cast<short>(port.bottom + 1), static_cast<short>(port.right + 1)};
        scroll_ = NewControl(window_, &bar, "\p", true, 0, 0, 0, scrollBarProc, 0);
    } else {
        ShowWindow(window_); // a closed card comes back, but never takes the focus
    }
    SetPortWindowPort(window_);
    sync_scroll();
    InvalRect(&window_->portRect);
    draw();
}
void InfoCardWindow::activate(bool active) {
    if (window_ && scroll_) {
        if (active)
            sync_scroll();
        else
            HiliteControl(scroll_, 255);
    }
}
void InfoCardWindow::draw() {
    if (!window_)
        return;
    SetPortWindowPort(window_);
    EraseRect(&window_->portRect);
    TextFont(0);
    TextSize(12);
    unsigned char line[64];
    TextFace(bold);
    to_pascal(line, card_.title);
    MoveTo(kMargin, 16);
    DrawString(line);
    TextFace(normal);
    const unsigned last = std::min(card_.line_count, offset_ + visible_lines());
    for (unsigned i = offset_; i < last; ++i) {
        to_pascal(line, card_.lines[i]);
        MoveTo(kMargin, static_cast<short>(kTop + kLine * (static_cast<short>(i - offset_) + 1) - 2));
        DrawString(line);
    }
    DrawControls(window_);
}
bool InfoCardWindow::mouse_down(const EventRecord& event) {
    if (!window_)
        return false;
    WindowPtr hit = nullptr;
    const auto part = FindWindow(event.where, &hit);
    if (hit != window_)
        return false;
    if (part == inGoAway) {
        if (TrackGoAway(window_, event.where))
            HideWindow(window_);
    } else if (part == inDrag) {
        Rect limit{20, -32000, 32000, 32000};
        DragWindow(window_, event.where, &limit);
    } else if (part == inContent) {
        if (FrontWindow() != window_) {
            SelectWindow(window_); // the first click only activates, as for a document window
            return true;
        }
        SetPortWindowPort(window_);
        Point local = event.where;
        GlobalToLocal(&local);
        ControlHandle control = nullptr;
        const auto control_part = FindControl(local, window_, &control);
        if (control == scroll_ && control_part != 0) {
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
            else if (tracked == inThumb)
                next = GetControlValue(scroll_);
            const unsigned most = card_.line_count > visible ? card_.line_count - visible : 0;
            offset_ = static_cast<unsigned>(std::clamp(next, 0, static_cast<int>(most)));
            sync_scroll();
            draw();
        }
    }
    return true;
}
} // namespace globe
