// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include <cmath>
#include <cstdint>
#include <limits>

#include "check.hpp"
#include "globe/trackball.hpp"

using namespace globe;

namespace {

// Deterministic generator so failures reproduce; not for anything statistical.
struct Lcg {
    std::uint32_t state;
    double next() { // [0, 1)
        state = state * 1664525U + 1013904223U;
        return static_cast<double>(state >> 8) / 16777216.0;
    }
};

Camera camera_at(double lat, double lon, double heading = 0.0, double radius = 200.0) {
    const auto cam = make_camera({{lat, lon}, heading, radius, 640, 480});
    CHECK(cam.has_value());
    return cam.value_or(Camera{});
}

double basis_error(const Camera& c) {
    double e = std::fabs(length(c.right) - 1.0) + std::fabs(length(c.up) - 1.0) +
               std::fabs(length(c.forward) - 1.0);
    e += std::fabs(dot(c.right, c.up)) + std::fabs(dot(c.right, c.forward)) +
         std::fabs(dot(c.up, c.forward));
    e += length(cross(c.right, c.up) - c.forward);
    return e;
}

double camera_distance(const Camera& a, const Camera& b) {
    return length(a.right - b.right) + length(a.up - b.up) + length(a.forward - b.forward);
}

// Point inside the globe's disc, at most `fraction` of the radius from the center.
ScreenPoint point_in_disc(const Camera& c, Lcg& rng, double fraction) {
    const double r = fraction * c.radius_px * std::sqrt(rng.next());
    const double a = 2.0 * kPi * rng.next();
    return {c.center_x + r * std::cos(a), c.center_y + r * std::sin(a)};
}

void test_unproject() {
    const Camera cam = camera_at(10.0, 20.0);
    const Vec3 c = unproject_to_sphere(cam, {cam.center_x, cam.center_y});
    CHECK_NEAR(c.x, 0.0, 1e-15);
    CHECK_NEAR(c.y, 0.0, 1e-15);
    CHECK_NEAR(c.z, 1.0, 1e-15);

    // Right edge of the disc is the limb; up is screen-up (smaller y).
    const Vec3 e = unproject_to_sphere(cam, {cam.center_x + cam.radius_px, cam.center_y});
    CHECK_NEAR(e.x, 1.0, 1e-12);
    CHECK_NEAR(e.z, 0.0, 1e-7);
    const Vec3 up = unproject_to_sphere(cam, {cam.center_x, cam.center_y - 100.0});
    CHECK(up.y > 0.0 && up.z > 0.0);

    // Outside the disc: clamped to the limb, unit length, same direction.
    const Vec3 out = unproject_to_sphere(cam, {cam.center_x + 3.0 * cam.radius_px, cam.center_y});
    CHECK_NEAR(out.x, 1.0, 1e-12);
    CHECK_NEAR(out.z, 0.0, 0.0);
    const Vec3 diag = unproject_to_sphere(cam, {cam.center_x - 500.0, cam.center_y - 500.0});
    CHECK_NEAR(length(diag), 1.0, 1e-12);
    CHECK(diag.x < 0.0 && diag.y > 0.0 && diag.z == 0.0);

    // Non-finite input never produces NaN.
    const Vec3 bad = unproject_to_sphere(cam, {std::nan(""), 0.0});
    CHECK(bad.x == 0.0 && bad.y == 0.0 && bad.z == 1.0);

    // Inverse of to_screen for points inside the disc.
    Lcg rng{1};
    for (int i = 0; i < 200; ++i) {
        const ScreenPoint p = point_in_disc(cam, rng, 0.99);
        const Vec3 v = unproject_to_sphere(cam, p);
        CHECK_NEAR(length(v), 1.0, 1e-12);
        const ScreenPoint q = to_screen(cam, v);
        CHECK_NEAR(q.x, p.x, 1e-9);
        CHECK_NEAR(q.y, p.y, 1e-9);
    }
}

// The defining property: the world point under `from` ends up under `to`.
void test_drag_grabs_the_point() {
    Lcg rng{42};
    const double lats[] = {-90, -60, -10, 0, 25, 75, 90};
    for (const double lat : lats) {
        for (int k = 0; k < 40; ++k) {
            const Camera start = camera_at(lat, 360.0 * rng.next() - 180.0, 360.0 * rng.next());
            const ScreenPoint from = point_in_disc(start, rng, 0.95);
            const ScreenPoint to = point_in_disc(start, rng, 0.95);
            const Vec3 grabbed_world = start.right * unproject_to_sphere(start, from).x +
                                       start.up * unproject_to_sphere(start, from).y +
                                       start.forward * unproject_to_sphere(start, from).z;
            const Camera moved = drag_rotate(start, from, to);
            CHECK(basis_error(moved) < 1e-12);
            const ScreenPoint landed = to_screen(moved, to_camera_space(moved, grabbed_world));
            CHECK_NEAR(landed.x, to.x, 1e-8);
            CHECK_NEAR(landed.y, to.y, 1e-8);
        }
    }
}

void test_drag_directions() {
    // At the equator, dragging the globe to the left brings eastern longitudes to the middle.
    const Camera cam = camera_at(0.0, 0.0);
    const Camera left = drag_rotate(cam, {320.0, 240.0}, {300.0, 240.0});
    const LatLon c = center_of(left);
    CHECK(c.lon_deg > 0.0);
    CHECK_NEAR(c.lat_deg, 0.0, 1e-9);
    // Dragging up brings southern latitudes to the middle.
    const Camera up = drag_rotate(cam, {320.0, 240.0}, {320.0, 220.0});
    CHECK(center_of(up).lat_deg < 0.0);
    CHECK_NEAR(center_of(up).lon_deg, 0.0, 1e-9);
    // Small drag angle matches the geometry: dx pixels at the center is asin(dx / radius).
    CHECK_NEAR(center_of(left).lon_deg, std::asin(20.0 / 200.0) * kRadToDeg, 1e-9);
}

void test_identity_and_invalid() {
    const Camera cam = camera_at(33.0, -70.0, 15.0);
    const Camera same = drag_rotate(cam, {300.0, 200.0}, {300.0, 200.0});
    CHECK(camera_distance(cam, same) == 0.0);
    const double inf = std::numeric_limits<double>::infinity();
    CHECK(camera_distance(cam, drag_rotate(cam, {std::nan(""), 0.0}, {1.0, 1.0})) == 0.0);
    CHECK(camera_distance(cam, drag_rotate(cam, {1.0, 1.0}, {0.0, inf})) == 0.0);
    // Opposite limb points: the shortest rotation is ill-defined; a half turn in the screen
    // plane is used, keeping the center fixed and still mapping the grabbed point to the target.
    const Camera half = drag_rotate(cam, {cam.center_x + cam.radius_px, cam.center_y},
                                    {cam.center_x - cam.radius_px, cam.center_y});
    CHECK(basis_error(half) < 1e-12);
    CHECK_NEAR(length(half.forward - cam.forward), 0.0, 1e-12);
    CHECK_NEAR(dot(half.right, cam.right), -1.0, 1e-12);
}

void test_reversible() {
    Lcg rng{7};
    for (int i = 0; i < 100; ++i) {
        const Camera start =
            camera_at(180.0 * rng.next() - 90.0, 360.0 * rng.next() - 180.0, 360.0 * rng.next());
        const ScreenPoint a = point_in_disc(start, rng, 0.9);
        const ScreenPoint b = point_in_disc(start, rng, 0.9);
        const Camera there = drag_rotate(start, a, b);
        const Camera back = drag_rotate(there, b, a);
        CHECK(camera_distance(start, back) < 1e-9);
    }
}

void test_no_drift() {
    // Many tiny drags, including near the poles, must leave a valid right-handed unit basis.
    Lcg rng{99};
    Camera cam = camera_at(89.0, 12.0);
    for (int i = 0; i < 200000; ++i) {
        const ScreenPoint a = point_in_disc(cam, rng, 0.98);
        const ScreenPoint b{a.x + (rng.next() - 0.5) * 3.0, a.y + (rng.next() - 0.5) * 3.0};
        cam = drag_rotate(cam, a, b);
    }
    CHECK(basis_error(cam) < 1e-12);
    CHECK(std::isfinite(center_of(cam).lat_deg) && std::isfinite(center_of(cam).lon_deg));
}

void test_drag_repairs_a_skewed_basis() {
    // Drag must re-orthonormalize, not merely avoid adding error: a slightly skewed input
    // (as from accumulated rounding) comes out as a valid right-handed unit basis.
    Camera skew = camera_at(20.0, 30.0, 10.0);
    skew.right = skew.right + skew.up * 1e-6;
    skew.up = skew.up * 1.000001;
    CHECK(basis_error(skew) > 1e-7);
    const Camera moved = drag_rotate(skew, {300.0, 200.0}, {310.0, 215.0});
    CHECK(basis_error(moved) < 1e-12);
}

void test_poles() {
    // Centered on the north pole, dragging works and moves the center off the pole.
    const Camera pole = camera_at(90.0, 0.0);
    CHECK_NEAR(center_of(pole).lat_deg, 90.0, 1e-9);
    const Camera moved = drag_rotate(pole, {320.0, 240.0}, {320.0, 200.0});
    CHECK(center_of(moved).lat_deg < 90.0);
    CHECK(basis_error(moved) < 1e-12);
    // Dragging over the pole keeps going: no clamp at +-90 as a lat/lon scheme would impose.
    Camera cam = camera_at(80.0, 0.0);
    for (int i = 0; i < 6; ++i) {
        cam = drag_rotate(cam, {320.0, 240.0}, {320.0, 280.0});
    }
    CHECK(basis_error(cam) < 1e-12);
    CHECK(center_of(cam).lat_deg < 80.0); // went over the top and down the far side
}

void test_orthonormalized() {
    Camera skew = camera_at(20.0, 30.0, 10.0);
    skew.right = skew.right + skew.up * 0.01;
    skew.up = skew.up * 1.02;
    skew.forward = skew.forward + skew.right * 0.005;
    const Camera fixed = orthonormalized(skew);
    CHECK(basis_error(fixed) < 1e-12);
    // Forward is kept (up to normalization).
    CHECK_NEAR(angle_between(fixed.forward, skew.forward), 0.0, 1e-12);
    // Degenerate basis is returned unchanged rather than filled with NaN.
    Camera flat = skew;
    flat.right = flat.forward;
    CHECK(camera_distance(orthonormalized(flat), flat) == 0.0);
    Camera zero = skew;
    zero.forward = {0.0, 0.0, 0.0};
    CHECK(camera_distance(orthonormalized(zero), zero) == 0.0);
}

void test_zoom() {
    const Camera cam = camera_at(0.0, 0.0, 0.0, 200.0);
    CHECK_NEAR(zoomed(cam, 1.5, 50.0, 2000.0).radius_px, 300.0, 1e-12);
    CHECK_NEAR(zoomed(cam, 0.5, 50.0, 2000.0).radius_px, 100.0, 1e-12);
    CHECK_NEAR(zoomed(cam, 0.01, 50.0, 2000.0).radius_px, 50.0, 0.0);
    CHECK_NEAR(zoomed(cam, 1000.0, 50.0, 2000.0).radius_px, 2000.0, 0.0);
    CHECK_NEAR(zoomed(cam, 0.0, 50.0, 2000.0).radius_px, 200.0, 0.0);
    CHECK_NEAR(zoomed(cam, -2.0, 50.0, 2000.0).radius_px, 200.0, 0.0);
    CHECK_NEAR(zoomed(cam, std::nan(""), 50.0, 2000.0).radius_px, 200.0, 0.0);
    CHECK_NEAR(zoomed(cam, std::numeric_limits<double>::infinity(), 50.0, 2000.0).radius_px, 200.0,
               0.0);
    // Zooming leaves orientation and the screen center alone.
    const Camera z = zoomed(cam, 2.0, 50.0, 2000.0);
    CHECK(camera_distance(cam, z) == 0.0);
    CHECK(z.center_x == cam.center_x && z.center_y == cam.center_y);
    // A camera already outside the limits is pulled in rather than left outside.
    CHECK_NEAR(zoomed(cam, 1.0, 300.0, 400.0).radius_px, 300.0, 0.0);
}

void test_center_of() {
    const double lats[] = {-90, -33, 0, 45, 90};
    for (const double lat : lats) {
        const LatLon c = center_of(camera_at(lat, 77.0, 40.0));
        CHECK_NEAR(c.lat_deg, lat, 1e-9);
        if (lat > -90.0 && lat < 90.0) {
            CHECK_NEAR(c.lon_deg, 77.0, 1e-9);
        }
    }
}

} // namespace

int main() {
    test_unproject();
    test_drag_grabs_the_point();
    test_drag_directions();
    test_identity_and_invalid();
    test_reversible();
    test_no_drift();
    test_drag_repairs_a_skewed_basis();
    test_poles();
    test_orthonormalized();
    test_zoom();
    test_center_of();
    return TEST_MAIN_RESULT();
}
