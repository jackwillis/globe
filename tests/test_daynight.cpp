// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/daynight.hpp"
#include <cmath>
#include <vector>
using namespace globe;
int main() {
    // March equinox 2026 (20 Mar 14:46 UTC): the sun is over the equator.
    CHECK(std::fabs(subsolar_point(1774017960).lat_deg) < 0.2);
    // June solstice 2026 (21 Jun 08:24 UTC): the Tropic of Cancer, near 23.44 N.
    const auto june = subsolar_point(1782030240);
    CHECK(std::fabs(june.lat_deg - 23.44) < 0.1);
    CHECK(std::fabs(june.lon_deg - (-(8.4 - 12.0) * 15.0 + 0.0)) < 5.0); // 08:24 UTC: about 54 E
    // 4 Nov 2026 12:00 UTC: the equation of time is about +16 minutes, so the sun is about 4
    // degrees west of Greenwich, with declination near -15.
    const auto nov = subsolar_point(1793793600);
    CHECK(std::fabs(nov.lat_deg - (-15.2)) < 0.4);
    CHECK(std::fabs(nov.lon_deg - (-4.1)) < 0.5);
    // A day later the sun is lower by about 0.2 degrees and the longitude has barely moved.
    CHECK(subsolar_point(1793793600 + 86400).lat_deg < nov.lat_deg);

    // Shading: the sun faces the viewer, so the center is unchanged; the far limb of a camera
    // looking the other way is dark; pixels outside the disc are never touched.
    auto fb = Framebuffer::create(120, 100);
    CHECK(fb);
    if (!fb)
        return 1;
    const Rgb565 white = 0xFFFF;
    auto camera = *make_camera({{0, 0}, 0, 40, 120, 100});
    fb->clear(white);
    shade_night(*fb, camera, to_unit_vector({0, 0})); // sun straight at the viewer
    CHECK(fb->pixel(60, 50) == white);
    CHECK(fb->pixel(3, 3) == white); // outside the disc
    fb->clear(white);
    shade_night(*fb, camera, to_unit_vector({0, 180})); // sun behind the globe: all night
    const auto dark = fb->pixel(60, 50);
    CHECK(dark != white);
    CHECK(((dark >> 11) & 31) < 14 && ((dark >> 5) & 63) < 28); // below about 0.4 of white
    CHECK(fb->pixel(3, 3) == white);
    // Sun over the east limb: the east half is day, the west half is night, with a gradient.
    fb->clear(white);
    shade_night(*fb, camera, to_unit_vector({0, 90}));
    CHECK(fb->pixel(90, 50) == white);
    CHECK(fb->pixel(30, 50) != white);
    const auto west = fb->pixel(30, 50), mid = fb->pixel(58, 50);
    CHECK(mid != white && ((mid >> 11) & 31) >= ((west >> 11) & 31)); // twilight is lighter
    // The terminator line alone: with the sun over the east limb it is the central meridian.
    fb->clear(white);
    CHECK(draw_terminator(*fb, camera, to_unit_vector({0, 90}), kTerminatorColor) > 40);
    CHECK(fb->pixel(60, 50) == kTerminatorColor || fb->pixel(61, 50) == kTerminatorColor);
    CHECK(fb->pixel(60, 30) == kTerminatorColor || fb->pixel(61, 30) == kTerminatorColor);
    CHECK(fb->pixel(40, 50) == white && fb->pixel(80, 50) == white); // no shading either side
    // Sun straight at the viewer: the terminator is the limb, nothing near the center.
    fb->clear(white);
    draw_terminator(*fb, camera, to_unit_vector({0, 0}), kTerminatorColor);
    CHECK(fb->pixel(60, 50) == white);
    // Sun behind the globe: the same great circle, so the limb again; a zero vector draws nothing.
    CHECK(draw_terminator(*fb, camera, {0, 0, 0}, kTerminatorColor) == 0);
    // Rows outside the framebuffer or a huge radius never write out of bounds.
    auto big = *make_camera({{0, 0}, 0, 4000, 120, 100});
    fb->clear(white);
    shade_night(*fb, big, to_unit_vector({0, 180}));
    CHECK(fb->pixel(0, 0) != white || fb->pixel(119, 99) != white);
    return TEST_MAIN_RESULT();
}
