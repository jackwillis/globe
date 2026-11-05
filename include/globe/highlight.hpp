// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Overlay for a focused country: its rings drawn as a bold line over a finished frame. The caller
// draws it on a copy of the frame that is shown, so clearing the focus needs no re-render.
#pragma once
#include "globe/countries.hpp"
#include "globe/framebuffer.hpp"
#include "globe/projection.hpp"
namespace globe {
// Draws every ring of country `id` that faces the viewer, about two pixels wide. Segments with an
// end behind the globe are skipped. Returns the number of segments drawn (0 for an unknown id).
std::size_t draw_country_outline(Framebuffer& fb, const Camera& camera, const Countries& countries,
                                 std::uint32_t id, Rgb565 color);
// Paints every other pixel (a checkerboard) inside the country in `color`, so the land underneath
// shows through. Rings are clipped to the visible hemisphere along the limb; holes and separate
// islands are handled by an even-odd fill. Returns the pixels painted; a country that would cover
// most of the disc (an unreliable clip) paints nothing and the outline alone marks it.
std::size_t fill_country_dither(Framebuffer& fb, const Camera& camera, const Countries& countries,
                                std::uint32_t id, Rgb565 color);
constexpr Rgb565 kCountryHighlight = rgb565(255, 90, 0);
} // namespace globe
