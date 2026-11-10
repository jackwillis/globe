// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "about_window.hpp"
#include "globe/countries.hpp"
#include "globe/strings.hpp"
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
void AboutWindow::set_language(Language language) {
    if (language == language_)
        return;
    language_ = language;
    if (!window_ || !shown_flag_)
        return;
    unsigned char title[256];
    const std::string_view text = tr(language_, Str::AboutTitle);
    title[0] = static_cast<unsigned char>(std::min<std::size_t>(text.size(), 255));
    for (unsigned char i = 0; i < title[0]; ++i)
        title[i + 1] = static_cast<unsigned char>(text[i]);
    SetWTitle(window_, title);
    draw();
}
bool AboutWindow::open(Language language) {
    language_ = language;
    if (!window_) {
        Rect bounds{70, 60, static_cast<short>(70 + kHeight), static_cast<short>(60 + kWidth)};
        const unsigned char title[] = {0};
        window_ = NewCWindow(nullptr, &bounds, title, false, noGrowDocProc,
                             reinterpret_cast<WindowPtr>(-1), true, 0);
        if (!window_)
            return false;
    }
    unsigned char title[256];
    const std::string_view text = tr(language_, Str::AboutTitle);
    title[0] = static_cast<unsigned char>(std::min<std::size_t>(text.size(), 255));
    for (unsigned char i = 0; i < title[0]; ++i)
        title[i + 1] = static_cast<unsigned char>(text[i]);
    SetWTitle(window_, title);
    ShowWindow(window_);
    shown_flag_ = true;
    SelectWindow(window_);
    SetPortWindowPort(window_);
    draw();
    return true;
}
void AboutWindow::close() {
    if (window_)
        HideWindow(window_);
    shown_flag_ = false;
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
    pascal_draw(tr(language_, Str::AboutName));
    TextSize(12);
    TextFace(normal);
    InfoCard card{};
    card.lines[0] = tr(language_, Str::AboutLine1);
    card.lines[1] = tr(language_, Str::AboutLine2);
    card.lines[2] = tr(language_, Str::AboutLine3);
    card.line_count = 3;
    const auto rows = wrap_card(card, kWidth - 2 * kMargin, [](std::string_view text) {
        return static_cast<int>(
            TextWidth(const_cast<char*>(text.data()), 0, static_cast<short>(text.size())));
    });
    for (unsigned i = 0; i < rows.count; ++i) {
        MoveTo(kMargin, static_cast<short>(52 + kLine * static_cast<short>(i)));
        pascal_draw(rows.rows[i]);
    }
    SetPort(saved);
}
} // namespace globe
