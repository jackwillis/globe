// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "prefs_window.hpp"
#include "globe/strings.hpp"
#include <Quickdraw.h>
#include <algorithm>
#include <string_view>
namespace globe {
namespace {
constexpr short kWidth = 300, kHeight = 168, kMargin = 16, kRowHeight = 20;
// Radio rows: y of the first row's top, then 2 rows per group. Group headings sit above.
constexpr short kUnitsTop = 36, kLanguageTop = 100;
void pascal_text(unsigned char* out, std::string_view text) {
    const auto n = static_cast<unsigned char>(std::min<std::size_t>(text.size(), 255));
    out[0] = n;
    for (unsigned char i = 0; i < n; ++i)
        out[i + 1] = static_cast<unsigned char>(text[i]);
}
void draw_text(std::string_view text) {
    unsigned char line[256];
    pascal_text(line, text);
    DrawString(line);
}
void radio(short left, short top, bool on, const char* label) {
    Rect dot{static_cast<short>(top + 4), left, static_cast<short>(top + 16),
             static_cast<short>(left + 12)};
    FrameOval(&dot);
    if (on) {
        InsetRect(&dot, 3, 3);
        PaintOval(&dot);
    }
    MoveTo(static_cast<short>(left + 20), static_cast<short>(top + 15));
    draw_text(label);
}
} // namespace
PrefsWindow::~PrefsWindow() {
    if (window_)
        DisposeWindow(window_);
}
bool PrefsWindow::open(const Preferences& current) {
    shown_ = current;
    if (!window_) {
        Rect bounds{110, 120, static_cast<short>(110 + kHeight), static_cast<short>(120 + kWidth)};
        const unsigned char title[] = {0};
        window_ = NewCWindow(nullptr, &bounds, title, false, noGrowDocProc,
                             reinterpret_cast<WindowPtr>(-1), true, 0);
        if (!window_)
            return false;
    }
    unsigned char title[256];
    pascal_text(title, tr(shown_.language, Str::PrefsTitle));
    SetWTitle(window_, title);
    ShowWindow(window_);
    shown_flag_ = true;
    SelectWindow(window_);
    draw();
    return true;
}
void PrefsWindow::close() {
    if (window_)
        HideWindow(window_);
    shown_flag_ = false;
}
void PrefsWindow::sync(const Preferences& current) {
    if (current == shown_)
        return;
    const bool retitle = current.language != shown_.language;
    shown_ = current;
    if (!window_ || !shown_flag_)
        return;
    if (retitle) {
        unsigned char title[256];
        pascal_text(title, tr(shown_.language, Str::PrefsTitle));
        SetWTitle(window_, title);
    }
    draw();
}
void PrefsWindow::draw() {
    if (!window_)
        return;
    GrafPtr saved;
    GetPort(&saved);
    SetPortWindowPort(window_);
    EraseRect(&window_->portRect);
    TextFont(0);
    TextSize(12);
    TextFace(bold);
    MoveTo(kMargin, static_cast<short>(kUnitsTop - 8));
    draw_text(tr(shown_.language, Str::PrefsUnits));
    MoveTo(kMargin, static_cast<short>(kLanguageTop - 8));
    draw_text(tr(shown_.language, Str::PrefsLanguage));
    TextFace(normal);
    const auto lang = shown_.language;
    radio(kMargin, kUnitsTop, shown_.units == Units::Metric, tr(lang, Str::UnitsMetric));
    radio(kMargin, kUnitsTop + kRowHeight, shown_.units == Units::Imperial,
          tr(lang, Str::UnitsImperial));
    radio(kMargin, kLanguageTop, lang == Language::English, tr(lang, Str::LangEnglish));
    radio(kMargin, kLanguageTop + kRowHeight, lang == Language::Spanish,
          tr(lang, Str::LangSpanish));
    SetPort(saved);
}
std::optional<Preferences> PrefsWindow::mouse_down(Point local) const {
    if (local.h < 0 || local.h > kWidth)
        return std::nullopt;
    Preferences next = shown_;
    const auto row = [&](short top) { return local.v >= top && local.v < top + kRowHeight; };
    if (row(kUnitsTop))
        next.units = Units::Metric;
    else if (row(kUnitsTop + kRowHeight))
        next.units = Units::Imperial;
    else if (row(kLanguageTop))
        next.language = Language::English;
    else if (row(kLanguageTop + kRowHeight))
        next.language = Language::Spanish;
    if (next == shown_)
        return std::nullopt;
    return next;
}
} // namespace globe
