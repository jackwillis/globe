// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Day/night terminator: where the sun is overhead, and a shading pass that darkens the night side
// of a finished frame. The caller applies it to the copy that is shown, so the rendered frame and
// every pin stay exactly as they were.
#pragma once
#include "globe/framebuffer.hpp"
#include "globe/geometry.hpp"
#include "globe/projection.hpp"
#include <cstddef>
#include <cstdint>
namespace globe {
// The point on Earth where the sun is at the zenith, from UNIX time in seconds (UTC). Low-precision
// solar position (about 0.1 degree), enough for a visible terminator.
LatLon subsolar_point(std::int64_t unix_seconds);
// Darkens pixels of the globe disc that face away from `sun_world` (a unit vector in world space),
// with a soft twilight edge. Pixels outside the disc are untouched.
void shade_night(Framebuffer& fb, const Camera& camera, Vec3 sun_world);
// Just the terminator, a thin line where the sun is on the horizon, for frames that move: much
// cheaper than shading every pixel of the disc. Returns the segments drawn.
std::size_t draw_terminator(Framebuffer& fb, const Camera& camera, Vec3 sun_world, Rgb565 color);
constexpr Rgb565 kTerminatorColor = rgb565(255, 214, 64);
} // namespace globe
