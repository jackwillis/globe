// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/compass_overlay.hpp"
#include "globe/shown_frame.hpp"
#include <array>
#include <cmath>
#include <cstdlib>
#include <utility>
#include <vector>

using namespace globe;

namespace {
constexpr Rgb565 kBackground = 0x1234;

Camera camera_for(double lat, double heading, int width, int height) {
    const auto camera = make_camera({{lat, 35}, heading, height * 0.45, width, height});
    if (!camera)
        std::abort();
    return *camera;
}

void check_bearing(double lat, double heading, double x, double y) {
    const auto north = compass_north(camera_for(lat, heading, 420, 315));
    CHECK(north && std::abs(north->x - x) < 1e-9 && std::abs(north->y - y) < 1e-9);
}

void check_stroke(Framebuffer& fb, ScreenPoint north) {
    using Pixel = std::pair<int, int>;
    constexpr std::array<Pixel, 4> cardinal{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
    fb.clear(kBackground);
    draw_compass_rose(fb, north);
    auto visited = Framebuffer::create(fb.width(), fb.height());
    if (!visited)
        std::abort();
    visited->clear(kBackground);
    int white = 0;
    const int top = fb.height() - compass_overlay_rows(fb.width(), fb.height());
    for (int y = 0; y < fb.height(); ++y) {
        for (int x = 0; x < fb.width(); ++x) {
            const auto pixel = fb.pixel(x, y);
            if (pixel == kBackground)
                continue;
            CHECK(x >= 5 && x <= 27 && y >= top && y < fb.height() - kProgressBarRows);
            CHECK(pixel == kProgressFill || pixel == kProgressEdge);
            if (pixel != kProgressFill)
                continue;
            ++white;
            // Every shared pixel edge around the white core is covered by the stroke.
            for (const auto& [dx, dy] : cardinal)
                CHECK(fb.pixel(x + dx, y + dy) != kBackground);
        }
    }
    constexpr int center_x = 16;
    const int center_y = fb.height() - kProgressBarRows - 13;
    CHECK(fb.pixel(center_x, center_y) == kProgressFill);
    std::vector<Pixel> connected{{center_x, center_y}};
    visited->set_pixel(center_x, center_y, kProgressFill);
    for (std::size_t i = 0; i < connected.size(); ++i) {
        const auto [x, y] = connected[i];
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (fb.pixel(x + dx, y + dy) != kProgressFill ||
                    visited->pixel(x + dx, y + dy) == kProgressFill)
                    continue;
                visited->set_pixel(x + dx, y + dy, kProgressFill);
                connected.emplace_back(x + dx, y + dy);
            }
        }
    }
    CHECK(white > 20 && connected.size() == static_cast<std::size_t>(white));
}

void check_frame(int width, int height, double heading) {
    auto fb = Framebuffer::create(width, height);
    auto expected = Framebuffer::create(width, height);
    auto rose = Framebuffer::create(width, height);
    const auto camera = camera_for(-35, heading, width, height);
    const auto north = compass_north(camera);
    if (!fb || !expected || !rose || !north)
        std::abort();
    rose->clear(kBackground);
    draw_compass_rose(*rose, *north);
    expected->clear(kBackground);
    draw_center_scale(*expected, camera);
    draw_progress_bar(*expected, 0.5);

    fb->clear(kBackground);
    Overlays overlays;
    ShownFrame shown;
    const auto& frame = shown.begin(overlays, *fb, camera, 0, false, 0.5);
    int changed = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const auto pixel = rose->pixel(x, y);
            if (pixel == kBackground) {
                CHECK(frame.pixel(x, y) == expected->pixel(x, y));
                continue;
            }
            ++changed;
            CHECK(pixel == kProgressFill || pixel == kProgressEdge);
            CHECK(y >= height - compass_overlay_rows(width, height));
            CHECK(y < height - kProgressBarRows && x >= 5 && x <= 27);
            CHECK(expected->pixel(x, y) == kBackground);
            CHECK(frame.pixel(x, y) == pixel);
        }
    }
    CHECK(changed > 40);
    shown.end();
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            CHECK(fb->pixel(x, y) == kBackground);
}
} // namespace

int main() {
    check_bearing(45, 0, 0, -1);
    check_bearing(-45, 90, -1, 0); // North pole is behind the visible hemisphere.
    check_bearing(0, 180, 0, 1);
    check_bearing(0, 270, 1, 0);
    check_bearing(89.999, 45, -std::sqrt(0.5), -std::sqrt(0.5));
    CHECK(!compass_north(camera_for(90, 20, 420, 315)));
    CHECK(!compass_north(camera_for(-90, 70, 420, 315)));
    auto tiny = Framebuffer::create(60, 40);
    if (!tiny)
        std::abort();
    tiny->clear(kBackground);
    draw_compass_rose(*tiny, {0, -1});
    CHECK(compass_overlay_rows(60, 40) == 0 && compass_overlay_rows(640, 20) == 0);
    for (int y = 0; y < tiny->height(); ++y)
        for (int x = 0; x < tiny->width(); ++x)
            CHECK(tiny->pixel(x, y) == kBackground);
    auto up = Framebuffer::create(180, 40);
    if (!up)
        std::abort();
    up->clear(kBackground);
    draw_compass_rose(*up, {0, -1});
    CHECK(up->pixel(16, 7) == kProgressFill && up->pixel(16, 27) == kBackground);
    constexpr double diagonal = 0.7071067811865476;
    // At along = 3 px, away from the joins, the white shaft is one pixel thick
    // both on the axis and along the diagonal. Adjacent samples are black.
    CHECK(up->pixel(16, 14) == kProgressFill && up->pixel(15, 14) == kProgressEdge &&
          up->pixel(17, 14) == kProgressEdge);
    check_stroke(*up, {diagonal, -diagonal});
    CHECK(up->pixel(18, 15) == kProgressFill && up->pixel(17, 14) == kProgressEdge &&
          up->pixel(19, 16) == kProgressEdge);
    for (int heading = 0; heading < 360; ++heading) {
        const auto north = compass_north(camera_for(0, heading, 180, 40));
        if (!north)
            std::abort();
        check_stroke(*up, *north);
    }
    check_frame(180, 34, 0);
    check_frame(180, 40, 180);
    check_frame(640, 480, 45);
    return TEST_MAIN_RESULT();
}
