// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include "globe/overlays.hpp"

namespace globe::overlay_detail {
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
} // namespace globe::overlay_detail
