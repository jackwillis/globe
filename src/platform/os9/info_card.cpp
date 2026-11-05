// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "info_card.hpp"
#include <Events.h>
#include <Quickdraw.h>
#include <algorithm>
namespace globe {
namespace {
constexpr short kWidth = 360, kLine = 14, kTop = 22, kMargin = 8;
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
void InfoCardWindow::show(const InfoCard& card, short main_right) {
    card_ = card;
    card_.line_count = std::min(card_.line_count, static_cast<unsigned>(card_.lines.size()));
    const short height =
        static_cast<short>(kTop + kLine * static_cast<short>(card_.line_count) + 8);
    if (window_) {
        SizeWindow(window_, kWidth, height, true);
        ShowWindow(window_);
        SelectWindow(window_);
    } else {
        Rect bounds{60, static_cast<short>(main_right + 12), static_cast<short>(60 + height),
                    static_cast<short>(main_right + 12 + kWidth)};
        unsigned char title[20];
        to_pascal(title, "Globe information");
        window_ = NewCWindow(nullptr, &bounds, title, true, documentProc,
                             reinterpret_cast<WindowPtr>(-1), true, 0);
    }
    if (window_) {
        SetPortWindowPort(window_);
        InvalRect(&window_->portRect);
        draw();
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
    for (unsigned i = 0; i < card_.line_count; ++i) {
        to_pascal(line, card_.lines[i]);
        MoveTo(kMargin, static_cast<short>(kTop + kLine * (static_cast<short>(i) + 1) - 2));
        DrawString(line);
    }
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
        SelectWindow(window_);
    }
    return true;
}
} // namespace globe
