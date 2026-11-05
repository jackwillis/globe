// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/measure.hpp"
#include <cmath>
#include <vector>
using namespace globe;
static bool near(double a, double b, double tolerance) {
    return std::fabs(a - b) <= tolerance;
}
int main() {
    // Known city pairs (great-circle, mean radius).
    const LatLon london{51.5074, -0.1278}, new_york{40.7128, -74.0060};
    const LatLon paris{48.8566, 2.3522}, tokyo{35.6762, 139.6503};
    CHECK(near(great_circle_km(london, new_york), 5570.0, 20.0));
    CHECK(near(great_circle_km(paris, tokyo), 9712.0, 25.0));
    CHECK(
        near(great_circle_km({0, 0}, {0, 180}), 20015.1, 1.0)); // antipodal: half the circumference
    CHECK(near(great_circle_km({90, 0}, {0, 77}), 10007.5, 1.0)); // pole to equator
    CHECK(great_circle_km(london, london) == 0.0);
    CHECK(near(great_circle_km(london, new_york), great_circle_km(new_york, london), 1e-9));
    CHECK(near(great_circle_km({0, 179}, {0, -179}), great_circle_km({0, 0}, {0, 2}),
               1e-6)); // dateline
    // Bearings.
    CHECK(near(initial_bearing_deg({0, 0}, {10, 0}), 0.0, 1e-9));
    CHECK(near(initial_bearing_deg({0, 0}, {0, 90}), 90.0, 1e-9));
    CHECK(near(initial_bearing_deg({0, 0}, {-10, 0}), 180.0, 1e-9));
    CHECK(near(initial_bearing_deg({0, 0}, {0, -90}), 270.0, 1e-9));
    CHECK(near(initial_bearing_deg(london, new_york), 288.0, 1.0)); // about WNW
    CHECK(initial_bearing_deg(london, london) == 0.0);

    // Overlay: the arc between two points on the visible side draws pixels and marks both ends.
    auto fb = Framebuffer::create(160, 120);
    CHECK(fb);
    if (!fb)
        return 1;
    const auto camera = *make_camera({{0, 0}, 0, 50, 160, 120});
    fb->clear(0);
    const auto drawn = draw_measure(*fb, camera, {0, -30}, {0, 30}, kMeasureColor);
    CHECK(drawn >= 90);
    std::size_t colored = 0;
    for (int i = 0; i < 160 * 120; ++i)
        colored += fb->data()[i] == kMeasureColor;
    CHECK(colored > 100);
    CHECK(fb->pixel(80, 60) == kMeasureColor); // the equator arc passes through the center
    // Points behind the globe draw nothing; antipodes have no unique arc but still mark ends.
    fb->clear(0);
    CHECK(draw_measure(*fb, camera, {0, 150}, {10, 170}, kMeasureColor) == 0);
    for (int i = 0; i < 160 * 120; ++i)
        CHECK(fb->data()[i] == 0);
    CHECK(draw_measure(*fb, camera, {0, 0}, {0, 180}, kMeasureColor) == 0);
    return TEST_MAIN_RESULT();
}
