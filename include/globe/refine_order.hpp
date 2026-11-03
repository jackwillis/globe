// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Circle-outward refinement for the settled terrain, from the view center. The final image never
// depends on the focus: the last pass redraws every visible cell in canonical order.
#pragma once
#include "globe/geometry.hpp"
#include "globe/projection.hpp"
#include <cstdint>

namespace globe {

constexpr unsigned kRefinePasses = 16;

// Per-view constants, computed once per refinement job (a handful of sqrt and a table; never per
// cell). Cells compare their distance against `limit`, with no sqrt, log, acos or division.
struct PassPlan {
    ScreenPoint focus{}; // finite, as used (the view center replaces a bad focus)
    // Squared-distance pass boundaries, ascending.
    double limit[kRefinePasses - 1] = {};

    // Pass of a screen position.
    unsigned pass_at(double x, double y) const;
};

PassPlan make_pass_plan(const Camera& camera, ScreenPoint focus);

} // namespace globe
