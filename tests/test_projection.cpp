// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include <cmath>
#include <limits>

#include "check.hpp"
#include "globe/projection.hpp"

using namespace globe;

namespace {

constexpr double kEps = 1e-12;

Camera camera_at(double lat, double lon, double heading = 0.0, int w = 640, int h = 480,
                 double radius = 200.0) {
    const auto cam = make_camera({{lat, lon}, heading, radius, w, h});
    CHECK(cam.has_value());
    return cam.value_or(Camera{});
}

void test_unit_vectors() {
    const Vec3 origin = to_unit_vector({0.0, 0.0});
    CHECK_NEAR(origin.x, 0.0, kEps);
    CHECK_NEAR(origin.y, 0.0, kEps);
    CHECK_NEAR(origin.z, 1.0, kEps);
    const Vec3 east = to_unit_vector({0.0, 90.0});
    CHECK_NEAR(east.x, 1.0, kEps);
    CHECK_NEAR(east.z, 0.0, kEps);
    const Vec3 north = to_unit_vector({90.0, 0.0});
    CHECK_NEAR(north.y, 1.0, kEps);

    // Antimeridian: 180 and -180 (and 540) are the same place.
    const Vec3 p = to_unit_vector({20.0, 180.0});
    const Vec3 q = to_unit_vector({20.0, -180.0});
    const Vec3 r = to_unit_vector({20.0, 540.0});
    CHECK_NEAR(length(p - q), 0.0, 1e-9);
    CHECK_NEAR(length(p - r), 0.0, 1e-9);

    // Poles do not depend on longitude.
    CHECK_NEAR(length(to_unit_vector({90.0, 10.0}) - to_unit_vector({90.0, -123.0})), 0.0, 1e-9);
    CHECK_NEAR(length(to_unit_vector({-90.0, 10.0}) - to_unit_vector({-90.0, 77.0})), 0.0, 1e-9);

    // Out-of-range latitude clamps; non-finite input yields the origin vector, never NaN.
    CHECK_NEAR(to_unit_vector({123.0, 0.0}).y, 1.0, kEps);
    const Vec3 bad = to_unit_vector({std::nan(""), 0.0});
    CHECK(bad.z == 1.0 && bad.x == 0.0 && bad.y == 0.0);

    // Always unit length, and round trips through from_unit_vector.
    for (int lat = -90; lat <= 90; lat += 15) {
        for (int lon = -180; lon < 180; lon += 20) {
            const Vec3 v = to_unit_vector({static_cast<double>(lat), static_cast<double>(lon)});
            CHECK_NEAR(length(v), 1.0, 1e-12);
            const LatLon back = from_unit_vector(v);
            CHECK_NEAR(back.lat_deg, lat, 1e-9);
            if (lat != 90 && lat != -90) {
                CHECK_NEAR(back.lon_deg, lon, 1e-9);
            }
        }
    }
    CHECK_NEAR(from_unit_vector({0.0, 1.0, 0.0}).lon_deg, 0.0, 0.0);
}

void test_camera_validation() {
    const CameraParams ok{{0.0, 0.0}, 0.0, 100.0, 64, 64};
    CHECK(make_camera(ok).has_value());
    CameraParams bad = ok;
    bad.radius_px = 0.0;
    CHECK(!make_camera(bad));
    bad = ok;
    bad.radius_px = -5.0;
    CHECK(!make_camera(bad));
    bad = ok;
    bad.width = 0;
    CHECK(!make_camera(bad));
    bad = ok;
    bad.height = kMaxViewportDimension + 1;
    CHECK(!make_camera(bad));
    bad = ok;
    bad.center.lat_deg = std::numeric_limits<double>::infinity();
    CHECK(!make_camera(bad));
    bad = ok;
    bad.heading_deg = std::nan("");
    CHECK(!make_camera(bad));
    bad = ok;
    bad.radius_px = std::nan("");
    CHECK(!make_camera(bad));
}

void test_center_maps_to_screen_center() {
    const double centers[][2] = {{0, 0},     {40.7, -74.0}, {-33.9, 151.2}, {89.0, 10.0},
                                 {-89.0, 0}, {0, 179.9},    {0, -180},      {90, 33}};
    for (const auto& c : centers) {
        const Camera cam = camera_at(c[0], c[1], 0.0, 640, 480);
        const Vec3 cp = to_camera_space(cam, to_unit_vector({c[0], c[1]}));
        CHECK_NEAR(cp.x, 0.0, 1e-12);
        CHECK_NEAR(cp.y, 0.0, 1e-12);
        CHECK_NEAR(cp.z, 1.0, 1e-12);
        const ScreenPoint s = to_screen(cam, cp);
        CHECK_NEAR(s.x, 320.0, 1e-9);
        CHECK_NEAR(s.y, 240.0, 1e-9);
    }
    // Odd sizes: the center is the continuous midpoint, not a pixel index.
    const Camera odd = camera_at(10.0, 20.0, 0.0, 641, 481);
    const ScreenPoint s = to_screen(odd, to_camera_space(odd, to_unit_vector({10.0, 20.0})));
    CHECK_NEAR(s.x, 320.5, 1e-9);
    CHECK_NEAR(s.y, 240.5, 1e-9);
}

void test_camera_basis_is_orthonormal_right_handed() {
    const double lats[] = {-90, -45, 0, 30, 90};
    const double lons[] = {-180, -90, 0, 45, 179};
    const double headings[] = {0, 30, 90, 200};
    for (const double lat : lats) {
        for (const double lon : lons) {
            for (const double hd : headings) {
                const Camera c = camera_at(lat, lon, hd);
                CHECK_NEAR(length(c.right), 1.0, 1e-12);
                CHECK_NEAR(length(c.up), 1.0, 1e-12);
                CHECK_NEAR(length(c.forward), 1.0, 1e-12);
                CHECK_NEAR(dot(c.right, c.up), 0.0, 1e-12);
                CHECK_NEAR(dot(c.right, c.forward), 0.0, 1e-12);
                CHECK_NEAR(dot(c.up, c.forward), 0.0, 1e-12);
                CHECK_NEAR(length(cross(c.right, c.up) - c.forward), 0.0, 1e-12);
            }
        }
    }
}

void test_orientation() {
    // North up: a point north of the center is above it, a point east is to the right.
    const Camera cam = camera_at(0.0, 0.0);
    const ScreenPoint n = to_screen(cam, to_camera_space(cam, to_unit_vector({10.0, 0.0})));
    CHECK(n.y < 240.0);
    CHECK_NEAR(n.x, 320.0, 1e-9);
    const ScreenPoint e = to_screen(cam, to_camera_space(cam, to_unit_vector({0.0, 10.0})));
    CHECK(e.x > 320.0);
    CHECK_NEAR(e.y, 240.0, 1e-9);
    // 90 degrees from the center lies on the limb, exactly radius_px away.
    const ScreenPoint limb = to_screen(cam, to_camera_space(cam, to_unit_vector({0.0, 90.0})));
    CHECK_NEAR(limb.x, 320.0 + 200.0, 1e-9);

    // Heading 90: east is shown toward the top of the screen.
    const Camera east_up = camera_at(0.0, 0.0, 90.0);
    const ScreenPoint ee =
        to_screen(east_up, to_camera_space(east_up, to_unit_vector({0.0, 10.0})));
    CHECK(ee.y < 240.0);
    CHECK_NEAR(ee.x, 320.0, 1e-9);

    // Viewed from the north pole centered on longitude 0, screen-up points away from that
    // meridian (north is the derivative of latitude, which at the pole heads toward -z), so a
    // point at lat 80 on lon 0 sits straight below the center.
    const Camera pole = camera_at(90.0, 0.0);
    const ScreenPoint p = to_screen(pole, to_camera_space(pole, to_unit_vector({80.0, 0.0})));
    CHECK_NEAR(p.x, 320.0, 1e-9);
    CHECK(p.y > 240.0);
}

void test_rear_hemisphere_rejected() {
    const Camera cam = camera_at(0.0, 0.0);
    CHECK(is_front(to_camera_space(cam, to_unit_vector({0.0, 0.0}))));
    CHECK(is_front(to_camera_space(cam, to_unit_vector({50.0, 60.0}))));
    CHECK(!is_front(to_camera_space(cam, to_unit_vector({0.0, 180.0}))));
    CHECK(!is_front(to_camera_space(cam, to_unit_vector({0.0, 91.0}))));
    CHECK(!is_front(to_camera_space(cam, to_unit_vector({0.0, -91.0}))));
    CHECK(!is_front(to_camera_space(cam, to_unit_vector({-60.0, 170.0}))));
    // Exactly on the limb counts as visible.
    CHECK(is_front({1.0, 0.0, 0.0}));
    CHECK(is_front({0.0, 1.0, 0.0}));
    CHECK(!is_front({0.0, 0.0, -1e-12}));
    // The antipode of the center is never visible, whatever the center.
    const double lats[] = {-80, -10, 0, 25, 80};
    for (const double lat : lats) {
        for (int lon = -180; lon < 180; lon += 45) {
            const Camera c = camera_at(lat, lon);
            CHECK(!is_front(to_camera_space(c, to_unit_vector({-lat, lon + 180.0}))));
        }
    }
}

void test_horizon_clipping() {
    const Camera cam = camera_at(0.0, 0.0);
    // Fully visible and fully hidden arcs.
    const Vec3 a = to_camera_space(cam, to_unit_vector({0.0, -30.0}));
    const Vec3 b = to_camera_space(cam, to_unit_vector({0.0, 30.0}));
    const auto whole = clip_arc_to_front(a, b);
    CHECK(whole.has_value());
    if (whole) {
        CHECK(length(whole->a - a) == 0.0 && length(whole->b - b) == 0.0);
    }
    const Vec3 h1 = to_camera_space(cam, to_unit_vector({0.0, 120.0}));
    const Vec3 h2 = to_camera_space(cam, to_unit_vector({20.0, 150.0}));
    CHECK(!clip_arc_to_front(h1, h2));

    // An equatorial arc from lon 45 to lon 135 crosses the limb at lon 90 exactly.
    const Vec3 v1 = to_camera_space(cam, to_unit_vector({0.0, 45.0}));
    const Vec3 v2 = to_camera_space(cam, to_unit_vector({0.0, 135.0}));
    const auto cut = clip_arc_to_front(v1, v2);
    CHECK(cut.has_value());
    if (cut) {
        CHECK(length(cut->a - v1) == 0.0);
        CHECK_NEAR(cut->b.z, 0.0, 1e-12);
        CHECK_NEAR(cut->b.x, 1.0, 1e-12);
        CHECK_NEAR(length(cut->b), 1.0, 1e-12);
    }
    // Reversing the arc reverses the result.
    const auto rev = clip_arc_to_front(v2, v1);
    CHECK(rev.has_value());
    if (rev) {
        CHECK_NEAR(rev->a.x, 1.0, 1e-12);
        CHECK_NEAR(rev->a.z, 0.0, 1e-12);
        CHECK(length(rev->b - v1) == 0.0);
    }

    // The crossing lies on the true great circle, not on the chord: for a long oblique arc the
    // clipped endpoint is coplanar with the original two (triple product zero) and unit length.
    const Vec3 s = to_camera_space(cam, to_unit_vector({60.0, -20.0}));
    const Vec3 t = to_camera_space(cam, to_unit_vector({-40.0, 160.0}));
    const auto oblique = clip_arc_to_front(s, t);
    CHECK(oblique.has_value());
    if (oblique) {
        CHECK_NEAR(dot(cross(s, t), oblique->b), 0.0, 1e-12);
        CHECK_NEAR(length(oblique->b), 1.0, 1e-12);
        CHECK_NEAR(oblique->b.z, 0.0, 1e-12);
        // The clipped endpoint lies between s and t on the shorter arc.
        CHECK_NEAR(angle_between(s, oblique->b) + angle_between(oblique->b, t), angle_between(s, t),
                   1e-12);
    }

    // Both endpoints on the limb: visible, not clipped away.
    const auto limb = clip_arc_to_front({1.0, 0.0, 0.0}, {0.0, 1.0, 0.0});
    CHECK(limb.has_value());

    // A near-antipodal pair has no defined shorter arc.
    CHECK(!clip_arc_to_front({0.0, 0.0, 1.0}, {0.0, 0.0, -1.0}));
}

void test_antimeridian_and_poles() {
    // An arc across the antimeridian is short in 3D: no discontinuity, no long way round.
    const Vec3 west = to_unit_vector({10.0, 179.0});
    const Vec3 east = to_unit_vector({10.0, -179.0});
    CHECK(angle_between(west, east) < 0.04);
    Vec3 pts[16];
    const std::size_t n = subdivide_arc(west, east, 1.0 * kDegToRad, pts, 16);
    CHECK(n >= 2);
    for (std::size_t i = 1; i < n; ++i) {
        CHECK(angle_between(pts[i - 1], pts[i]) <= 1.0 * kDegToRad + 1e-12);
    }
    // A camera centered on the antimeridian sees both sides continuously.
    const Camera cam = camera_at(10.0, 180.0);
    const ScreenPoint sw = to_screen(cam, to_camera_space(cam, west));
    const ScreenPoint se = to_screen(cam, to_camera_space(cam, east));
    CHECK(sw.x < 320.0 && se.x > 320.0);
    CHECK(std::fabs(sw.x - se.x) < 20.0);

    // Over the pole: an arc between opposite meridians passes near the pole, still continuous.
    const Vec3 a = to_unit_vector({80.0, 0.0});
    const Vec3 b = to_unit_vector({80.0, 180.0});
    CHECK_NEAR(angle_between(a, b), 20.0 * kDegToRad, 1e-9);
    const std::size_t m = subdivide_arc(a, b, 5.0 * kDegToRad, pts, 16);
    CHECK(m == 5);
    if (m == 5) {
        CHECK_NEAR(pts[2].y, 1.0, 1e-9); // midpoint of the arc is the pole itself
    }
}

void test_subdivision() {
    Vec3 pts[64];
    const Vec3 a = to_unit_vector({0.0, 0.0});
    const Vec3 b = to_unit_vector({0.0, 90.0});

    // 90 degrees at <= 10 degrees per step: 9 steps, 10 points, endpoints preserved exactly.
    const std::size_t n = subdivide_arc(a, b, 10.0 * kDegToRad, pts, 64);
    CHECK(n == 10);
    if (n == 10) {
        CHECK(length(pts[0] - a) == 0.0 && length(pts[9] - b) == 0.0);
        for (std::size_t i = 0; i < n; ++i) {
            CHECK_NEAR(length(pts[i]), 1.0, 1e-12);
            CHECK_NEAR(pts[i].y, 0.0, 1e-12); // stays on the equator
            CHECK_NEAR(angle_between(a, pts[i]), static_cast<double>(i) * 10.0 * kDegToRad, 1e-9);
        }
    }
    // A step larger than the arc yields just the two endpoints.
    CHECK(subdivide_arc(a, b, 3.0, pts, 64) == 2);
    // Identical endpoints are still a valid (zero-length) arc of two points.
    CHECK(subdivide_arc(a, a, 0.1, pts, 64) == 2);

    // Failure modes write nothing beyond capacity.
    CHECK(subdivide_arc(a, b, 0.0, pts, 64) == 0);
    CHECK(subdivide_arc(a, b, -1.0, pts, 64) == 0);
    CHECK(subdivide_arc(a, b, std::nan(""), pts, 64) == 0);
    CHECK(subdivide_arc(a, b, 1.0 * kDegToRad, pts, 5) == 0); // would need 91 points
    CHECK(subdivide_arc(a, b, 10.0 * kDegToRad, pts, 1) == 0);
    CHECK(subdivide_arc(a, b, 10.0 * kDegToRad, nullptr, 64) == 0);
    CHECK(subdivide_arc({0.0, 0.0, 1.0}, {0.0, 0.0, -1.0}, 0.1, pts, 64) == 0); // antipodal
    Vec3 guard[4] = {{7, 7, 7}, {7, 7, 7}, {7, 7, 7}, {7, 7, 7}};
    CHECK(subdivide_arc(a, b, 1.0 * kDegToRad, guard, 3) == 0);
    CHECK(guard[0].x == 7 && guard[3].x == 7);
    // Exactly-fitting capacity succeeds without overrun: 90 deg / 45 deg = 2 steps = 3 points.
    Vec3 exact[3];
    CHECK(subdivide_arc(a, b, 45.0 * kDegToRad, exact, 3) == 3);
}

} // namespace

int main() {
    test_unit_vectors();
    test_camera_validation();
    test_center_maps_to_screen_center();
    test_camera_basis_is_orthonormal_right_handed();
    test_orientation();
    test_rear_hemisphere_rejected();
    test_horizon_clipping();
    test_antimeridian_and_poles();
    test_subdivision();
    return TEST_MAIN_RESULT();
}
