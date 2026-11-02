// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Virtual trackball on top of the orthographic Camera. The camera is already a full orientation
// (right/up/forward basis), so dragging never stores latitude, longitude or heading: it rotates
// the basis so the world point grabbed at the start of a drag follows the pointer. Heading is
// therefore whatever the drags have made it, as with a physical globe.
//
// All functions are pure, total on finite input, and return the camera unchanged when given
// non-finite coordinates.
#pragma once

#include "globe/projection.hpp"

namespace globe {

// Maps a screen position to a camera-space unit vector on the visible hemisphere: inside the
// globe's disc it is the point under the pointer (z > 0); outside the disc it is the nearest
// limb point (z == 0), so dragging beyond the edge turns the globe in the screen plane.
// Non-finite input yields (0, 0, 1).
Vec3 unproject_to_sphere(const Camera& camera, ScreenPoint p);

// Rotates the camera so the world point that was under `from` is now under `to`, by the
// shortest rotation. The result is re-orthonormalized, so thousands of drags do not accumulate
// error. Returns the camera unchanged when from == to or either point is not finite.
Camera drag_rotate(const Camera& camera, ScreenPoint from, ScreenPoint to);

// Gram-Schmidt on the basis, keeping forward fixed and the frame right-handed. Returns the
// input unchanged if the basis is degenerate.
Camera orthonormalized(const Camera& camera);

// Multiplies the globe radius by factor, clamped to [min_radius_px, max_radius_px]. A factor
// that is not finite and positive leaves the camera unchanged.
Camera zoomed(const Camera& camera, double factor, double min_radius_px, double max_radius_px);

// Geographic point at the viewport center.
LatLon center_of(const Camera& camera);

} // namespace globe
