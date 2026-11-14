// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "about_window.hpp"
#include "globe/countries.hpp"
#include "globe/strings.hpp"
#include "globe/version.hpp"
#include "globe/wrap.hpp"
#include <Quickdraw.h>
#include <algorithm>
namespace globe {
namespace {
constexpr short kWidth = 460, kHeight = 350, kMargin = 14, kLine = 15;
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
        const unsigned char title[] = {0};
        window_ = NewCWindow(nullptr, &bounds, title, false, noGrowDocProc,
                             reinterpret_cast<WindowPtr>(-1), true, 0);
        if (!window_)
            return false;
    }
    unsigned char title[256];
    const std::string_view text = tr(Str::AboutTitle);
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
    pascal_draw(tr(Str::AboutName));
    TextSize(12);
    TextFace(normal);
    InfoCard card{};
    // The card borrows this buffer through wrapping and drawing.
    static_assert(kVersion.size() < 255);
    constexpr std::size_t kMaxVersionLabel = 255 - 1 - kVersion.size();
    const auto label = tr(Str::AboutVersion).substr(0, kMaxVersionLabel);
    char version_line[255];
    auto* version_start = std::copy(label.begin(), label.end(), version_line);
    *version_start++ = ' ';
    const auto* version_end = std::copy(kVersion.begin(), kVersion.end(), version_start);
    card.lines[0] =
        std::string_view(version_line, static_cast<std::size_t>(version_end - version_line));
    card.lines[1] = tr(Str::AboutLine1);
    card.lines[2] = tr(Str::AboutLine2);
    card.lines[3] = tr(Str::AboutLine3);
    card.lines[4] = tr(Str::AboutLine4);
    card.lines[5] = tr(Str::AboutLine5);
    card.lines[6] = tr(Str::AboutLine6);
    card.line_count = 7;
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
