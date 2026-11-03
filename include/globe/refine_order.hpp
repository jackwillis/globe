// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Focus-outward refinement orders for the settled terrain. The final image never depends on the
// order or the focus: the last pass redraws every visible cell in canonical order.
#pragma once
#include "globe/geometry.hpp"
#include "globe/projection.hpp"
#include <cstdint>

namespace globe {

// Rectangle: expanding rectangles shaped like the viewport, reaching its edges together.
// Circle: expanding circles (equal screen distance). Globe: rings of equal great-circle angle.
enum class RefinementOrder : std::uint8_t { Rectangle, Circle, Globe };
constexpr unsigned kRefinePasses = 16;

// Per-view constants, computed once per refinement job (a handful of sqrt and a table; never per
// cell). Cells compare their distance against `limit`, with no sqrt, log, acos or division.
struct PassPlan {
    RefinementOrder order = RefinementOrder::Circle;
    double width = 1.0, height = 1.0; // viewport size in pixels
    ScreenPoint focus{};              // finite, as used (the view center replaces a bad focus)
    Vec3 focus_world{};               // Globe: unit vector of the focus on the sphere
    // Pass boundaries, ascending distance for Rectangle/Circle, descending cosine for Globe.
    double limit[kRefinePasses - 1] = {};

    // Pass of a screen position, for Rectangle and Circle (Globe uses pass_for_dot).
    unsigned pass_at(double x, double y) const;
    // Pass of a cell whose (mean) dot product with the focus unit vector is `d`.
    unsigned pass_for_dot(double d) const;
};

PassPlan make_pass_plan(const Camera& camera, RefinementOrder order, ScreenPoint focus);

} // namespace globe
