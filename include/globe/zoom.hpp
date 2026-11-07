// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Cheap zoom preview. An orthographic globe zoomed about its center is the same picture scaled
// about that point, so a finished frame can stand in for the real render while the wheel turns.
#pragma once
#include "globe/framebuffer.hpp"
#include <vector>
namespace globe {
// Nearest-neighbor copy of `src` scaled by `factor` about (cx, cy) into `dst` (same size). A pixel
// whose source lies outside `src` gets `outside` if it is beyond `disc_radius` of the center and
// `hole` if it is inside the disc (the preview has nothing to show there). Returns the number of
// hole pixels, or kInvalidZoom for a size mismatch or a non-finite or non-positive argument.
// `columns` is caller-owned scratch (resized once per size, so steady state does not allocate).
constexpr std::size_t kInvalidZoom = static_cast<std::size_t>(-1);
std::size_t scale_frame(const Framebuffer& src, Framebuffer& dst, double cx, double cy,
                        double factor, Rgb565 outside, Rgb565 hole, double disc_radius,
                        std::vector<int>& columns);
} // namespace globe
