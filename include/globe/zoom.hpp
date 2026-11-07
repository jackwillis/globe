// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Cheap zoom preview. An orthographic globe zoomed about its center is the same picture scaled
// about that point, so a finished frame can stand in for the real render while the wheel turns.
#pragma once
#include "globe/framebuffer.hpp"
#include <vector>
namespace globe {
// Nearest-neighbor copy of `src` scaled by `factor` about (cx, cy) into `dst` (same size). Pixels
// whose source lies outside `src` get `fill`. Returns false if such a pixel lies within
// `disc_radius` of the center in `dst`: the preview would show a hole in the globe.
// `columns` is caller-owned scratch (resized once per size, so steady state does not allocate).
bool scale_frame(const Framebuffer& src, Framebuffer& dst, double cx, double cy, double factor,
                 Rgb565 fill, double disc_radius, std::vector<int>& columns);
} // namespace globe
