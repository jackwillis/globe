// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/scale_overlay.hpp"
#include "globe/overlays.hpp"
#include <algorithm>
#include <cstdint>
namespace globe {
namespace {
constexpr int kGlyphWidth = 5, kGlyphHeight = 7, kAdvance = 6;
constexpr int kMinWidth = 80, kMaxBarWidth = 120;
constexpr int kMinTickSpacing = 4;                              // pixels between ticks
constexpr int kEndReach = 3, kMediumReach = 2, kSmallReach = 1; // rows either side of the line
// Rows from the bottom: the progress bar, a gap, then the scale's bar line (ticks reach 1 row
// either side) and the label above it.
constexpr int kBarLift = kProgressBarRows + 6; // rows from the bottom edge to the bar line
constexpr int kLabelGap = 6;                   // from the bar line up to the label's bottom row
struct Glyph {
    char ch;
    std::uint8_t rows[kGlyphHeight]; // five bits a row, left pixel the high bit
};
constexpr Glyph kGlyphs[] = {
    {'0', {0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e}},
    {'1', {0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e}},
    {'2', {0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f}},
    {'3', {0x1e, 0x01, 0x01, 0x0e, 0x01, 0x01, 0x1e}},
    {'4', {0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02}},
    {'5', {0x1f, 0x10, 0x1e, 0x01, 0x01, 0x11, 0x0e}},
    {'6', {0x06, 0x08, 0x10, 0x1e, 0x11, 0x11, 0x0e}},
    {'7', {0x1f, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e}},
    {'9', {0x0e, 0x11, 0x11, 0x0f, 0x01, 0x02, 0x0c}},
    {',', {0x00, 0x00, 0x00, 0x00, 0x06, 0x02, 0x04}},
    {'k', {0x10, 0x10, 0x12, 0x14, 0x18, 0x14, 0x12}},
    {'m', {0x00, 0x00, 0x1a, 0x15, 0x15, 0x15, 0x15}},
};
const Glyph* glyph_for(char ch) {
    for (const auto& g : kGlyphs)
        if (g.ch == ch)
            return &g;
    return nullptr;
}
// Columns a character takes, including the gap after it: the comma is narrow.
int advance(char ch) {
    return ch == ',' ? 3 : kAdvance;
}
// Pixel size of the type: doubled where the frame is wide enough to carry it.
int type_scale(int width) {
    return width >= 360 ? 2 : 1;
}
struct Layout {
    int type, bar_width, bar_y, label_top;
    double value; // the length in the unit shown
    char text[16];
    int length;
};
// The label "N km" or "N m" ("500 m" below one kilometre), spelled out here so the core needs no
// number formatting.
int format_distance(const CenterScaleBar& bar, char* out) {
    int n = 0;
    // Rounded to the nearest whole number by hand (Retro68 68k has no std::round).
    const auto whole = static_cast<long>(bar.value);
    const long value = whole + (bar.value - static_cast<double>(whole) >= 0.5 ? 1 : 0);
    char digits[12];
    int count = 0;
    for (long v = value; v > 0 && count < 11; v /= 10)
        digits[count++] = static_cast<char>('0' + v % 10);
    if (count == 0)
        digits[count++] = '0';
    while (count > 0) {
        out[n++] = digits[--count];
        if (count > 0 && count % 3 == 0)
            out[n++] = ','; // 1,000 and up
    }
    out[n++] = ' ';
    switch (bar.unit) {
    case LengthUnit::Kilometer:
        out[n++] = 'k';
        out[n++] = 'm';
        break;
    case LengthUnit::Meter:
        out[n++] = 'm';
        break;
    case LengthUnit::Mile:
        out[n++] = 'm';
        out[n++] = 'i';
        break;
    case LengthUnit::Foot:
        out[n++] = 'f';
        out[n++] = 't';
        break;
    }
    out[n] = 0;
    return n;
}
bool lay_out(int width, int height, const Camera& camera, Units units, Layout& layout) {
    if (width < kMinWidth)
        return false;
    layout.type = type_scale(width);
    const int top = height - kBarLift - kLabelGap -
                    std::max(kGlyphHeight * layout.type, LabelBitmap::kMaxHeight) - 1;
    if (top < 0)
        return false;
    const auto bar = center_scale_bar(camera.radius_px, scale_bar_max_width(width), units);
    if (!bar || bar->width_px < 2)
        return false;
    layout.bar_width = bar->width_px;
    layout.value = bar->value;
    layout.bar_y = height - kBarLift;
    layout.label_top = top + 1;
    layout.length = format_distance(*bar, layout.text);
    return true;
}
// White pixels with a black edge: every pixel is put twice, as a 3x3 black block and then white.
struct Pen {
    Framebuffer& fb;
    void edge(int x, int y) {
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx)
                fb.set_pixel(x + dx, y + dy, kProgressEdge);
    }
    void fill(int x, int y) { fb.set_pixel(x, y, kProgressFill); }
};
template <class Shape> void stroke(Pen& pen, Shape&& shape) {
    shape([&](int x, int y) { pen.edge(x, y); });
    shape([&](int x, int y) { pen.fill(x, y); });
}
} // namespace
ScaleTicks scale_ticks(double value) {
    if (!(value > 0.0))
        return {};
    double mantissa = value;
    while (mantissa >= 9.5)
        mantissa /= 10.0;
    while (mantissa < 0.95)
        mantissa *= 10.0;
    ScaleTicks ticks;
    ticks.units = mantissa < 1.5 ? 1 : mantissa < 3.5 ? 2 : 5;
    ticks.steps_per_unit = ticks.units == 1 ? 10 : 2;
    return ticks;
}
int scale_bar_max_width(int width) {
    return std::clamp(width / 3, 0, kMaxBarWidth);
}
int scale_label_text(const Camera& camera, int width, char* out, int capacity, Units units) {
    if (width < kMinWidth || capacity < 16)
        return 0;
    const auto bar = center_scale_bar(camera.radius_px, scale_bar_max_width(width), units);
    return bar && bar->width_px >= 2 ? format_distance(*bar, out) : 0;
}
int scale_overlay_rows(int width, int height) {
    if (width < kMinWidth)
        return 0;
    const int label = std::max(kGlyphHeight * type_scale(width), LabelBitmap::kMaxHeight);
    const int top = height - kBarLift - kLabelGap - label - 2;
    return top < 0 ? 0 : height - top;
}
void draw_center_scale(Framebuffer& fb, const Camera& camera, const LabelBitmap* label,
                       Units units) {
    Layout layout{};
    if (!lay_out(fb.width(), fb.height(), camera, units, layout))
        return;
    Pen pen{fb};
    const int x0 = (fb.width() - layout.bar_width) / 2, x1 = x0 + layout.bar_width - 1;
    const int y = layout.bar_y;
    stroke(pen, [&](auto put) {
        for (int x = x0; x <= x1; ++x)
            put(x, y);
        const auto tick = [&](int x, int reach) {
            for (int dy = -reach; dy <= reach; ++dy)
                put(x, y + dy);
        };
        tick(x0, kEndReach);
        tick(x1, kEndReach);
        // Medium ticks at the whole units (the halfway one on a 1), small ones between; each kind
        // is left out when its ticks would crowd.
        const auto ticks = scale_ticks(layout.value);
        const int steps = ticks.steps(), span = layout.bar_width - 1;
        const bool small = span / steps >= kMinTickSpacing;
        const bool medium = span / (ticks.units > 1 ? ticks.units : 2) >= kMinTickSpacing;
        for (int i = 1; i < steps; ++i) {
            const bool whole = ticks.medium(i);
            if (whole ? medium : small)
                tick(x0 + (i * span + steps / 2) / steps, whole ? kMediumReach : kSmallReach);
        }
    });
    if (label != nullptr && label->bits != nullptr && label->width > 0 && label->height > 0 &&
        label->height <= LabelBitmap::kMaxHeight && label->stride * 8 >= label->width) {
        // The same place as the built-in label: centered, its last row just above the bar.
        const int left = (fb.width() - label->width) / 2;
        const int top = layout.bar_y - 6 - label->height;
        stroke(pen, [&](auto put) {
            for (int row = 0; row < label->height; ++row)
                for (int col = 0; col < label->width; ++col)
                    if ((label->bits[row * label->stride + col / 8] >> (7 - col % 8)) & 1)
                        put(left + col, top + row);
        });
        return;
    }
    const int s = layout.type;
    int columns = 0;
    for (int i = 0; i < layout.length; ++i)
        columns += advance(layout.text[i]);
    int x = (fb.width() - (columns * s - s)) / 2;
    for (int i = 0; i < layout.length; x += advance(layout.text[i++]) * s) {
        const Glyph* g = glyph_for(layout.text[i]);
        if (g == nullptr)
            continue;
        stroke(pen, [&](auto put) {
            for (int row = 0; row < kGlyphHeight; ++row)
                for (int col = 0; col < kGlyphWidth; ++col)
                    if ((g->rows[row] >> (kGlyphWidth - 1 - col)) & 1)
                        for (int dy = 0; dy < s; ++dy)
                            for (int dx = 0; dx < s; ++dx)
                                put(x + col * s + dx, layout.label_top + row * s + dy);
        });
    }
}
} // namespace globe
