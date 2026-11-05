// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Great-circle measuring: distance and bearing between two points, and an overlay that draws the
// arc between them on the shown copy of a frame (the rendered frame is never changed).
#pragma once
#include "globe/framebuffer.hpp"
#include "globe/geometry.hpp"
#include "globe/projection.hpp"
#include <cstddef>
namespace globe {
constexpr double kEarthMeanRadiusKm = 6371.0088;
// Haversine distance on a sphere of the mean Earth radius, in kilometers (within about 0.5 percent
// of the ellipsoidal distance).
double great_circle_km(LatLon a, LatLon b);
// Initial compass bearing from a to b in degrees [0, 360). 0 at coincident points.
double initial_bearing_deg(LatLon a, LatLon b);
// Draws the short great-circle arc a-b (where it faces the viewer), about two pixels wide, with a
// small disc on each end. Returns the number of segments drawn.
std::size_t draw_measure(Framebuffer& fb, const Camera& camera, LatLon a, LatLon b, Rgb565 color);
// A single marker at `a` (the first point before the second is chosen).
void draw_measure_point(Framebuffer& fb, const Camera& camera, LatLon a, Rgb565 color);
constexpr Rgb565 kMeasureColor = rgb565(220, 0, 160);
} // namespace globe
