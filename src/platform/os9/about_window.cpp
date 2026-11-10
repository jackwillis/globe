// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "about_window.hpp"
#include "globe/countries.hpp"
#include "globe/wrap.hpp"
#include <Quickdraw.h>
#include <algorithm>
namespace globe {
namespace {
constexpr short kWidth = 360, kHeight = 170, kMargin = 14, kLine = 15;
void pascal_draw(std::string_view text) {
    unsigned char line[256];
    const auto n = static_cast<unsigned char>(std::min<std::size_t>(text.size(), 255));
    line[0] = n;
    for (unsigned char i = 0; i < n; ++i)
        line[i + 1] = static_cast<unsigned char>(text[i]);
    DrawString(line);
}
} // namespace
AboutWindow::~AboutWindow() {
    if (window_)
        DisposeWindow(window_);
}
bool AboutWindow::open() {
    if (!window_) {
        Rect bounds{70, 60, static_cast<short>(70 + kHeight), static_cast<short>(60 + kWidth)};
        const unsigned char title[] = {11, 'A', 'b', 'o', 'u', 't', ' ', 'G', 'l', 'o', 'b', 'e'};
        window_ = NewCWindow(nullptr, &bounds, title, false, noGrowDocProc,
                             reinterpret_cast<WindowPtr>(-1), true, 0);
        if (!window_)
            return false;
    }
    ShowWindow(window_);
    SelectWindow(window_);
    SetPortWindowPort(window_);
    draw();
    return true;
}
void AboutWindow::close() {
    if (window_)
        HideWindow(window_);
}
void AboutWindow::draw() {
    if (!window_)
        return;
    GrafPtr saved;
    GetPort(&saved);
    SetPortWindowPort(window_);
    EraseRect(&window_->portRect);
    TextFont(0);
    TextSize(14);
    TextFace(bold);
    MoveTo(kMargin, 28);
    pascal_draw("Globe");
    TextSize(12);
    TextFace(normal);
    InfoCard card{};
    card.lines[0] = "An offline, software-rendered interactive Earth.";
    card.lines[1] = "Copyright (c) 2026 Jack Willis. GPL-3.0-or-later.";
    card.lines[2] = "Data sources and notices: see the ReadMe.";
    card.line_count = 3;
    const auto rows = wrap_card(card, kWidth - 2 * kMargin, [](std::string_view text) {
        return static_cast<int>(TextWidth(const_cast<char*>(text.data()), 0,
                                          static_cast<short>(text.size())));
    });
    for (unsigned i = 0; i < rows.count; ++i) {
        MoveTo(kMargin, static_cast<short>(52 + kLine * static_cast<short>(i)));
        pascal_draw(rows.rows[i]);
    }
    SetPort(saved);
}
} // namespace globe
