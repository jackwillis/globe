// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/daynight.hpp"
#include "globe/overlays.hpp"
#include <cstdio>
#include <string>
#include <vector>
using namespace globe;
static std::vector<std::uint8_t> read(const std::string& path) {
    std::vector<std::uint8_t> b;
    if (auto* f = std::fopen(path.c_str(), "rb")) {
        int c;
        while ((c = std::fgetc(f)) != EOF)
            b.push_back(static_cast<std::uint8_t>(c));
        std::fclose(f);
    }
    return b;
}
static ScreenPoint at(const Camera& camera, double lat, double lon) {
    return to_screen(camera, to_camera_space(camera, to_unit_vector({lat, lon})));
}
int main(int argc, char** argv) {
    const std::string root = argc > 1 ? argv[1] : ".";
    auto bytes = read(root + "/tests/data/countries.gpol");
    auto countries = Countries::parse(bytes.data(), bytes.size());
    CHECK(countries);
    if (!countries)
        return 1;
    auto scene = CountryScene::create(*countries);
    CHECK(scene);
    if (!scene)
        return 1;
    const auto camera = *make_camera({{0, 26}, 0, 90, 160, 120});
    auto fb = Framebuffer::create(160, 120);
    CHECK(fb);
    if (!fb)
        return 1;
    fb->clear(0x1234);

    Overlays o;
    o.attach(&*countries, &*scene, nullptr, nullptr);
    CHECK(!o.active() && o.compose(*fb, camera, 0) == nullptr);
    // A click inside the country focuses it, offers its card once, and draws the overlay.
    CHECK(o.click(camera, at(camera, 0, 25)));
    CHECK(o.focused() == 2u && o.active());
    const auto card = o.take_card();
    CHECK(card && card->title == "Hole country");
    CHECK(!o.take_card());
    auto* shown = o.compose(*fb, camera, 0);
    CHECK(shown && shown != &*fb);
    std::size_t changed = 0;
    for (int i = 0; i < 160 * 120; ++i)
        changed += shown->data()[i] != 0x1234;
    CHECK(changed > 100);
    for (int i = 0; i < 160 * 120; ++i)
        CHECK(fb->data()[i] == 0x1234); // the rendered frame is untouched
    // Quick frames (dragging, refining) outline the country but skip the dither fill.
    {
        auto* quick_frame = o.compose(*fb, camera, 0, true);
        CHECK(quick_frame != nullptr);
        std::size_t quick_changed = 0;
        for (int i = 0; i < 160 * 120; ++i)
            quick_changed += quick_frame->data()[i] != 0x1234;
        CHECK(quick_changed > 20 && quick_changed < changed); // the outline only
    }
    // Clicking the sea, or off the globe, clears the focus (or does nothing).
    CHECK(!o.click(camera, {1, 1})); // off the globe: no change
    CHECK(o.focused() == 2u);
    CHECK(o.click(camera, at(camera, 0, 60)) && !o.focused() && !o.active());
    CHECK(o.compose(*fb, camera, 0) == nullptr);

    // Measure mode: the first click marks a point, the second yields the distance card.
    o.toggle_measure();
    CHECK(o.measuring() && o.click(camera, at(camera, 0, 10)) && o.active());
    CHECK(!o.take_card());
    CHECK(o.click(camera, at(camera, 0, 40)));
    const auto distance = o.take_card();
    CHECK(distance && distance->title == "Distance" && distance->line_count == 4);
    CHECK(distance &&
          distance->lines[2].find("3336 km") != std::string_view::npos); // 30 deg of arc
    CHECK(o.compose(*fb, camera, 0) != nullptr);
    CHECK(o.click(camera, at(camera, 0, 20)) && !o.take_card()); // a third click starts over
    o.toggle_measure();                                          // leaving clears the points
    CHECK(!o.active());

    // Night: composes without any click, and the time is the caller's.
    o.toggle_night();
    CHECK(o.night() && o.active());
    fb->clear(0xFFFF);
    // 1793793600 is 4 Nov 2026 12:00 UTC: the sun is near Greenwich, so the east limb is night.
    auto* day = o.compose(*fb, camera, 1793793600);
    CHECK(day != nullptr);
    const auto noon = day->pixel(80, 60);
    CHECK(noon == 0xFFFF); // the middle of this view is in daylight
    // Quick frames (moving, refining) draw only the terminator line, not the shading.
    fb->clear(0xFFFF);
    auto* quick = o.compose(*fb, camera, 1793793600, true);
    CHECK(quick != nullptr);
    std::size_t line = 0, shaded = 0;
    for (int i = 0; i < 160 * 120; ++i) {
        line += quick->data()[i] == kTerminatorColor;
        shaded += quick->data()[i] != 0xFFFF && quick->data()[i] != kTerminatorColor;
    }
    CHECK(line > 40 && shaded == 0);
    o.toggle_night();
    CHECK(o.compose(*fb, camera, 1793793600) == nullptr);
    // The progress bar is opaque and minimal: only the filled part of the bottom ten rows changes,
    // a black edge on top and white below; nothing above it, nothing right of it.
    {
        for (const double fraction : {0.0, 0.5, 1.0}) {
            fb->clear(0x1234);
            draw_progress_bar(*fb, fraction);
            const int filled = static_cast<int>(fraction * 160);
            std::size_t changed = 0;
            for (int y = 0; y < 120; ++y)
                for (int x = 0; x < 160; ++x) {
                    const auto v = fb->pixel(x, y);
                    changed += v != 0x1234;
                    if (y < 110 || x >= filled)
                        CHECK(v == 0x1234);
                    else
                        CHECK(v == (y == 110 ? kProgressEdge : kProgressFill));
                }
            CHECK(changed == static_cast<std::size_t>(filled) * 10);
        }
        Overlays bar;
        bar.attach(&*countries, &*scene, nullptr, nullptr);
        CHECK(bar.compose(*fb, camera, 0, true) == nullptr); // the bar alone never copies the frame
    }
    return TEST_MAIN_RESULT();
}
