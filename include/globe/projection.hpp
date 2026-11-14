// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Orthographic globe camera: world unit vector -> camera space -> screen, plus horizon clipping
// and geodesic subdivision. Pure geometry; no I/O, no platform calls, no allocation.
//
// Camera space: +x right, +y up, +z toward the viewer. A point is on the visible hemisphere when
// z >= 0 (points exactly on the horizon, z == 0, count as visible). Screen space is continuous
// with the origin at the top-left corner of the viewport and y growing downward; pixel (i, j)
// covers [i, i+1) x [j, j+1), so the viewport center is (width / 2, height / 2).
#pragma once

#include "globe/prefs.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>

#include "globe/geometry.hpp"

namespace globe {

constexpr int kMaxViewportDimension = 4096;

struct CameraParams {
    LatLon center;      // geographic point shown at the viewport center
    double heading_deg; // compass direction shown toward screen-up; 0 = north up, 90 = east up
    double radius_px;   // projected radius of the globe, > 0
    int width;          // viewport size in pixels, 1..kMaxViewportDimension
    int height;
};

struct Camera {
    Vec3 right;   // world-space direction of camera +x
    Vec3 up;      // world-space direction of camera +y
    Vec3 forward; // world-space direction of camera +z (toward the viewer)
    double radius_px;
    double center_x; // screen-space position of the globe center
    double center_y;
    int width;
    int height;
};

struct ScreenPoint {
    double x;
    double y;
};

enum class LengthUnit : std::uint8_t { Kilometer, Meter, Mile, Foot };

struct CenterScaleBar {
    double distance_km; // the true length, whatever unit is shown
    int width_px;
    double value = 0.0; // the length in `unit`: 5 for "5 mi", 500 for "500 m"
    LengthUnit unit = LengthUnit::Kilometer;
};

// Uses the local orthographic scale at the view center. Selects the largest 1–2–5
// distance that fits max_width_px, and rounds the resulting length to the nearest pixel. The
// distance is 1, 2 or 5 times a power of ten in the unit shown: kilometres, or metres under one;
// with Units::Imperial miles, or feet under one mile.
std::optional<CenterScaleBar> center_scale_bar(double radius_px, int max_width_px,
                                               Units units = Units::Metric);

// True if both cameras show the same view of the globe at the same viewport and center, at any
// radius: only the zoom differs.
bool same_orientation(const Camera& a, const Camera& b);

// Fails (nullopt) on non-finite values, non-positive radius or out-of-range viewport size.
std::optional<Camera> make_camera(const CameraParams& params);

Vec3 to_camera_space(const Camera& camera, Vec3 world);

// Orthographic projection of a camera-space point. Does not test visibility.
ScreenPoint to_screen(const Camera& camera, Vec3 camera_point);

constexpr bool is_front(Vec3 camera_point) {
    return camera_point.z >= 0.0;
}

// The shorter great-circle arc between two unit vectors.
struct Arc {
    Vec3 a;
    Vec3 b;
};

// Clips the arc to the visible hemisphere (z >= 0) in camera space. The crossing point is exact
// on the great circle, not a chord. Returns nullopt when no part of the arc is visible, and also
// for a degenerate (near-antipodal) arc, whose shorter path is undefined.
std::optional<Arc> clip_arc_to_front(Vec3 a, Vec3 b);

// Writes unit vectors along the shorter great-circle arc from a to b into out, including both
// endpoints, so that consecutive points are at most max_step_rad apart (to within rounding, about
// 1e-9 relative). Returns the number of points written, or 0 if the arc is degenerate,
// max_step_rad is not positive and finite, or capacity is too small. Never writes more than
// capacity points.
//
// Currently used by tests as an exact reference; the renderer subdivides on its own with a cheaper
// method that avoids sin, atan2 and sqrt.
std::size_t subdivide_arc(Vec3 a, Vec3 b, double max_step_rad, Vec3* out, std::size_t capacity);

} // namespace globe
