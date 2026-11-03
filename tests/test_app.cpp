// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Usage: test_app [REPO_ROOT]
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "atlas_util.hpp"
#include "check.hpp"
#include "globe/app.hpp"
#include "globe/trace.hpp"
#include "globe/trackball.hpp"

using namespace globe;

namespace {

Model fresh() {
    return initial_model(160, 120);
}

double basis_error(const Camera& c) {
    double e = std::fabs(length(c.right) - 1.0) + std::fabs(length(c.up) - 1.0) +
               std::fabs(length(c.forward) - 1.0);
    e += std::fabs(dot(c.right, c.up)) + std::fabs(dot(c.right, c.forward)) +
         std::fabs(dot(c.up, c.forward));
    return e + length(cross(c.right, c.up) - c.forward);
}

bool same_camera(const Camera& a, const Camera& b) {
    return std::memcmp(&a.radius_px, &b.radius_px, sizeof(double)) == 0 &&
           length(a.right - b.right) == 0.0 && length(a.up - b.up) == 0.0 &&
           length(a.forward - b.forward) == 0.0 && a.width == b.width && a.height == b.height &&
           a.center_x == b.center_x && a.center_y == b.center_y;
}

void test_initial_model() {
    const Model m = fresh();
    CHECK(m.refinement_order == RefinementOrder::Circle); // the default order
    CHECK(m.camera.width == 160 && m.camera.height == 120);
    CHECK_NEAR(m.camera.radius_px, 0.4 * 120.0, 1e-12);
    CHECK_NEAR(m.camera.center_x, 80.0, 0.0);
    CHECK_NEAR(m.camera.center_y, 60.0, 0.0);
    CHECK(basis_error(m.camera) < 1e-12);
    CHECK(m.stage == QualityStage::Moving && !m.dragging && m.last_input_ms == 0);
    CHECK(m.layers.graticule && m.layers.coastlines && m.layers.borders);
    const Model huge = initial_model(100000, -5);
    CHECK(huge.camera.width == kMaxViewportDimension && huge.camera.height == 1);
}

void test_drag() {
    Model m = fresh();
    m.drag_mode = DragMode::Trackball; // exact grab-the-point behavior is the trackball's
    const Camera start = m.camera;
    // Moving without a button held does nothing.
    Effects fx = update(m, PointerMove{10, 90.0, 60.0});
    CHECK(!fx.redraw && same_camera(m.camera, start) && m.last_input_ms == 0);

    fx = update(m, PointerDown{20, 80.0, 60.0});
    CHECK(!fx.redraw && m.dragging && m.last_input_ms == 20);
    fx = update(m, PointerMove{36, 70.0, 60.0});
    CHECK(fx.redraw && m.last_input_ms == 36);
    CHECK(same_camera(m.camera, drag_rotate(start, {80.0, 60.0}, {70.0, 60.0})));
    // Same position again: nothing changed, so no redraw is requested.
    fx = update(m, PointerMove{40, 70.0, 60.0});
    CHECK(!fx.redraw);
    // Successive moves compose.
    const Camera mid = m.camera;
    fx = update(m, PointerMove{50, 60.0, 62.0});
    CHECK(fx.redraw && same_camera(m.camera, drag_rotate(mid, {70.0, 60.0}, {60.0, 62.0})));

    fx = update(m, PointerUp{60});
    CHECK(fx.redraw && !m.dragging && m.last_input_ms == 60); // redraws to restore the graticule
    const Camera after = m.camera;
    fx = update(m, PointerMove{70, 20.0, 20.0});
    CHECK(!fx.redraw && same_camera(m.camera, after));

    // Non-finite coordinates are ignored, even mid-drag, and do not poison later drags.
    update(m, PointerDown{80, 80.0, 60.0});
    fx = update(m, PointerMove{90, std::nan(""), 5.0});
    CHECK(!fx.redraw && same_camera(m.camera, after));
    fx = update(m, PointerMove{100, 75.0, 60.0});
    CHECK(fx.redraw && basis_error(m.camera) < 1e-12);
    update(m, PointerDown{110, INFINITY, 1.0});
    CHECK(m.dragging); // still dragging from before: the bad down was ignored
}

// North is up when the camera's right vector is horizontal (no y component) and up points north.
bool north_is_up(const Camera& c) {
    return std::fabs(c.right.y) < 1e-12 && c.up.y > 0.0;
}

void test_turntable_drag() {
    Model m = fresh();
    CHECK(m.drag_mode == DragMode::Turntable);
    CHECK(north_is_up(m.camera));
    const LatLon start = center_of(m.camera);
    const double deg_per_px = 180.0 / kPi / m.camera.radius_px;

    // Dragging right by 10 px turns the globe so the center moves west by 10 px of arc.
    update(m, PointerDown{0, 80.0, 60.0});
    Effects fx = update(m, PointerMove{16, 90.0, 60.0});
    CHECK(fx.redraw);
    CHECK_NEAR(center_of(m.camera).lon_deg, start.lon_deg - 10.0 * deg_per_px, 1e-9);
    CHECK_NEAR(center_of(m.camera).lat_deg, start.lat_deg, 1e-9);
    CHECK(north_is_up(m.camera));
    // Dragging down brings northern latitudes toward the middle.
    update(m, PointerMove{32, 90.0, 70.0});
    CHECK_NEAR(center_of(m.camera).lat_deg, start.lat_deg + 10.0 * deg_per_px, 1e-9);
    CHECK(north_is_up(m.camera));
    update(m, PointerUp{48});

    // The rim behaves like the middle: the same pixel move turns the globe the same amount,
    // wherever on the window (even off the globe) it happens.
    double turned[3];
    const double xs[] = {80.0, 120.0, 150.0}; // center, near the rim (radius 48), outside
    for (int i = 0; i < 3; ++i) {
        Model t = fresh();
        update(t, PointerDown{0, xs[i], 60.0});
        update(t, PointerMove{16, xs[i] - 5.0, 60.0});
        turned[i] = center_of(t.camera).lon_deg - start.lon_deg;
    }
    CHECK_NEAR(turned[0], turned[1], 1e-12);
    CHECK_NEAR(turned[0], turned[2], 1e-12);
    CHECK(turned[0] > 0.0);

    // Zooming in slows the turn in proportion.
    Model z = fresh();
    update(z, Wheel{0, 7}); // radius scales by 1.1^7
    const double r_ratio = z.camera.radius_px / fresh().camera.radius_px;
    update(z, PointerDown{1, 80.0, 60.0});
    update(z, PointerMove{2, 70.0, 60.0});
    CHECK_NEAR((center_of(z.camera).lon_deg - start.lon_deg) * r_ratio, turned[0] * 2.0, 1e-9);

    // The camera never rolls, however it is dragged; near the poles it neither jumps nor flips.
    Model r = fresh();
    update(r, PointerDown{0, 80.0, 60.0});
    std::uint32_t seed = 3;
    double x = 80.0;
    double y = 60.0;
    for (int i = 0; i < 2000; ++i) {
        seed = seed * 1664525U + 1013904223U;
        x += (static_cast<double>(seed >> 16) / 65536.0 - 0.5) * 30.0;
        seed = seed * 1664525U + 1013904223U;
        y += (static_cast<double>(seed >> 16) / 65536.0 - 0.5) * 30.0;
        update(r, PointerMove{static_cast<std::uint64_t>(i), x, y});
        CHECK(north_is_up(r.camera));
        CHECK(basis_error(r.camera) < 1e-12);
        const double lat = center_of(r.camera).lat_deg;
        CHECK(lat <= 89.9 + 1e-9 && lat >= -89.9 - 1e-9);
    }

    // Dragging a long way toward a pole stops at the latitude limit and keeps the longitude.
    Model p = fresh();
    const double lon0 = center_of(p.camera).lon_deg;
    update(p, PointerDown{0, 80.0, 60.0});
    update(p, PointerMove{1, 80.0, 1.0e6});
    CHECK_NEAR(center_of(p.camera).lat_deg, 89.9, 1e-9);
    CHECK_NEAR(center_of(p.camera).lon_deg, lon0, 1e-6);
    update(p, PointerMove{2, 80.0, -1.0e6});
    CHECK_NEAR(center_of(p.camera).lat_deg, -89.9, 1e-9);
    CHECK(north_is_up(p.camera));

    // Longitude wraps freely.
    Model w = fresh();
    update(w, PointerDown{0, 80.0, 60.0});
    update(w, PointerMove{1, 80.0 - 360.0 * (w.camera.radius_px * kPi / 180.0), 60.0});
    CHECK_NEAR(std::fabs(center_of(w.camera).lon_deg), 0.0, 1e-6);
    CHECK(basis_error(w.camera) < 1e-12);

    // A move to the same place changes nothing and asks for no redraw.
    Model same = fresh();
    update(same, PointerDown{0, 80.0, 60.0});
    CHECK(!update(same, PointerMove{1, 80.0, 60.0}).redraw);
}

void test_align_north_and_mode_toggle() {
    // In trackball mode a drag can leave north tilted; N puts it back without moving the center.
    Model m = fresh();
    update(m, CommandIssued{0, Command::ToggleDragMode});
    CHECK(m.drag_mode == DragMode::Trackball);
    update(m, PointerDown{1, 130.0, 60.0});
    update(m, PointerMove{2, 120.0, 90.0});
    update(m, PointerMove{3, 60.0, 100.0});
    update(m, PointerMove{4, 40.0, 40.0});
    update(m, PointerUp{5});
    CHECK(!north_is_up(m.camera)); // the roll the trackball allows
    const LatLon c = center_of(m.camera);
    const double r = m.camera.radius_px;
    Effects fx = update(m, CommandIssued{6, Command::AlignNorth});
    CHECK(fx.redraw && north_is_up(m.camera));
    CHECK_NEAR(center_of(m.camera).lat_deg, c.lat_deg, 1e-9);
    CHECK_NEAR(center_of(m.camera).lon_deg, c.lon_deg, 1e-9);
    CHECK_NEAR(m.camera.radius_px, r, 0.0);
    CHECK(basis_error(m.camera) < 1e-12);
    // Already aligned: nothing to redraw.
    CHECK(!update(m, CommandIssued{7, Command::AlignNorth}).redraw);
    // The toggle changes mode without redrawing, and back.
    fx = update(m, CommandIssued{8, Command::ToggleDragMode});
    CHECK(!fx.redraw && m.drag_mode == DragMode::Turntable);
    update(m, CommandIssued{9, Command::ToggleDragMode});
    CHECK(m.drag_mode == DragMode::Trackball);
    // After switching to the turntable, the first drag also straightens a tilted view.
    update(m, PointerDown{10, 130.0, 60.0});
    update(m, PointerMove{11, 120.0, 90.0});
    update(m, PointerMove{12, 60.0, 100.0});
    CHECK(!north_is_up(m.camera));
    update(m, PointerUp{13});
    update(m, CommandIssued{14, Command::ToggleDragMode});
    update(m, PointerDown{1000, 80.0, 60.0});
    update(m, PointerMove{1001, 84.0, 60.0}); // past the drag slop
    CHECK(north_is_up(m.camera));
}

void test_double_click_keeps_north_up_in_turntable() {
    Model m = fresh();
    update(m, PointerDown{0, 110.0, 40.0});
    update(m, PointerUp{10});
    update(m, PointerDown{100, 110.0, 40.0});
    CHECK(north_is_up(m.camera) && m.camera.radius_px > fresh().camera.radius_px);
}

void test_grid_hidden_while_dragging() {
    Model m = fresh();
    CHECK(effective_layers(m).graticule);
    update(m, PointerDown{0, 80.0, 60.0});
    CHECK(effective_layers(m).graticule); // a press alone changes nothing
    CHECK(effective_layers(m).coastlines && effective_layers(m).borders);
    Effects fx = update(m, PointerMove{16, 70.0, 60.0}); // a drag: hidden from here on
    CHECK(m.layers.graticule);                           // the user's setting is untouched
    CHECK(fx.redraw && !effective_layers(m).graticule);
    fx = update(m, PointerUp{32});
    CHECK(fx.redraw && effective_layers(m).graticule); // back, with a redraw to show it

    // A press and release with no motion drew nothing, so there is nothing to restore.
    update(m, PointerDown{1000, 80.0, 60.0});
    fx = update(m, PointerUp{1010});
    CHECK(!fx.redraw && effective_layers(m).graticule);
    // A drag that changed nothing visible (same point) also needs no restore.
    update(m, PointerDown{2000, 80.0, 60.0});
    update(m, PointerMove{2010, 80.0, 60.0});
    CHECK(!update(m, PointerUp{2020}).redraw);
    // With the graticule switched off there is nothing to bring back.
    update(m, CommandIssued{3000, Command::ToggleGraticule});
    update(m, PointerDown{4000, 80.0, 60.0});
    update(m, PointerMove{4010, 60.0, 60.0});
    CHECK(!update(m, PointerUp{4020}).redraw);
    CHECK(!effective_layers(m).graticule && !m.layers.graticule);
    // Other layers follow the user's switches during a drag too.
    update(m, CommandIssued{5000, Command::ToggleBorders});
    update(m, PointerDown{6000, 80.0, 60.0});
    CHECK(!effective_layers(m).borders && effective_layers(m).coastlines);
}

void test_double_click() {
    // Click twice quickly on the globe, off-center: the clicked point ends up centered, zoomed in.
    {
        Model m = fresh();
        const Camera start = m.camera;
        const ScreenPoint p{110.0, 50.0}; // inside the disc (radius 48 around 80,60)
        CHECK(std::hypot(p.x - 80.0, p.y - 60.0) < start.radius_px);
        const ScreenPoint p2{p.x + 2.0, p.y - 1.0}; // the second press, within the slop
        update(m, PointerDown{1000, p.x, p.y});
        update(m, PointerUp{1050});
        const Effects fx = update(m, PointerDown{1200, p2.x, p2.y});
        CHECK(fx.redraw && !m.dragging);
        CHECK_NEAR(m.camera.radius_px, 2.0 * start.radius_px, 1e-9);
        // The point under the second press (the one that triggers the action) is now centered.
        const Vec3 under = unproject_to_sphere(start, p2);
        const Vec3 world = start.right * under.x + start.up * under.y + start.forward * under.z;
        const ScreenPoint where = to_screen(m.camera, to_camera_space(m.camera, world));
        CHECK_NEAR(where.x, 80.0, 1e-6);
        CHECK_NEAR(where.y, 60.0, 1e-6);
        CHECK(basis_error(m.camera) < 1e-12);
        // Moving with the button still held does not drag.
        const Camera held = m.camera;
        CHECK(!update(m, PointerMove{1210, 40.0, 40.0}).redraw && same_camera(m.camera, held));
        update(m, PointerUp{1220});
        // A third quick press is a new first click, not another double click.
        const Camera before = m.camera;
        update(m, PointerDown{1300, 80.0, 60.0});
        CHECK(same_camera(m.camera, before) && m.dragging);
    }
    // Too slow, too far apart, or a drag in between: just two ordinary presses.
    {
        Model m = fresh();
        const Camera start = m.camera;
        update(m, PointerDown{0, 90.0, 60.0});
        update(m, PointerUp{10});
        update(m, PointerDown{kDoubleClickMs + 1, 90.0, 60.0}); // 1 ms late: a new first press
        CHECK(same_camera(m.camera, start) && m.dragging);
        update(m, PointerUp{450});
        update(m, PointerDown{1000, 90.0, 60.0}); // 599 ms after the last press: also plain
        CHECK(same_camera(m.camera, start));
        update(m, PointerUp{1010});
        update(m, PointerDown{1100, 90.0 + kDoubleClickSlopPx + 1.0, 60.0}); // quick but too far
        CHECK(same_camera(m.camera, start));
        update(m, PointerUp{1110});
        // Exactly at the limits still counts.
        update(m, PointerDown{3000, 90.0, 60.0});
        update(m, PointerUp{3010});
        update(m, PointerDown{3000 + kDoubleClickMs, 90.0 + kDoubleClickSlopPx, 60.0});
        CHECK(m.camera.radius_px > start.radius_px);
    }
    {
        Model m = fresh();
        const Camera start = m.camera;
        update(m, PointerDown{0, 90.0, 60.0});
        update(m, PointerMove{10, 130.0, 60.0}); // a real drag
        update(m, PointerUp{20});
        const double radius_after_drag = m.camera.radius_px;
        update(m, PointerDown{100, 130.0, 60.0}); // quickly pressed where the drag ended
        CHECK(m.camera.radius_px == radius_after_drag && m.dragging);
        (void)start;
    }
    {
        // Off the globe (the second press): no zoom, ordinary drag start.
        Model m = fresh();
        const Camera start = m.camera;
        update(m, PointerDown{0, 5.0, 5.0});
        update(m, PointerUp{10});
        update(m, PointerDown{100, 5.0, 5.0});
        CHECK(same_camera(m.camera, start) && m.dragging);
    }
    {
        // At the zoom limit the globe still recenters; time going backwards is not a double click.
        Model m = fresh();
        update(m, Wheel{0, 40});
        const double max_r = m.camera.radius_px;
        update(m, PointerDown{1000, 100.0, 60.0}); // inside the (huge) disc
        update(m, PointerUp{1010});
        const LatLon before = center_of(m.camera);
        update(m, PointerDown{1100, 100.0, 60.0});
        CHECK_NEAR(m.camera.radius_px, max_r, 1e-9);
        CHECK(center_of(m.camera).lon_deg != before.lon_deg ||
              center_of(m.camera).lat_deg != before.lat_deg);
        Model n = fresh();
        update(n, PointerDown{5000, 80.0, 60.0});
        update(n, PointerUp{5010});
        update(n, PointerDown{4000, 80.0, 60.0}); // earlier timestamp
        CHECK(n.dragging && n.camera.radius_px == fresh().camera.radius_px);
    }
    {
        // A lost release event: the second press arrives while still "dragging". It must end the
        // drag, so later motion does not rotate the globe that was just zoomed.
        Model m = fresh();
        update(m, PointerDown{0, 90.0, 60.0});
        update(m, PointerDown{100, 90.0, 60.0}); // no PointerUp in between
        CHECK(!m.dragging && m.camera.radius_px > fresh().camera.radius_px);
        const Camera zoomed_cam = m.camera;
        CHECK(!update(m, PointerMove{110, 20.0, 20.0}).redraw && same_camera(m.camera, zoomed_cam));
    }
    // A non-finite press neither starts a drag nor counts as a click.
    {
        Model m = fresh();
        update(m, PointerDown{0, std::nan(""), 60.0});
        CHECK(!m.dragging && !m.click_valid);
    }
}

void test_zoom_and_limits() {
    Model m = fresh();
    const double r0 = m.camera.radius_px;
    Effects fx = update(m, Wheel{1, 1});
    CHECK(fx.redraw && m.camera.radius_px > r0);
    CHECK_NEAR(m.camera.radius_px, r0 * 1.1, 1e-9);
    fx = update(m, Wheel{2, -1});
    CHECK(fx.redraw && std::fabs(m.camera.radius_px - r0) < 1e-9);
    update(m, Wheel{3, 1000});
    CHECK_NEAR(m.camera.radius_px, max_radius_px(160, 120), 1e-9);
    fx = update(m, Wheel{4, 1});
    CHECK(!fx.redraw); // at the limit: no pointless redraw
    update(m, Wheel{5, -1000});
    CHECK_NEAR(m.camera.radius_px, min_radius_px(160, 120), 1e-9);
    fx = update(m, Wheel{6, -1});
    CHECK(!fx.redraw);
    fx = update(m, Wheel{7, 0});
    CHECK(!fx.redraw);
    // Orientation is untouched by zooming.
    const Model n = fresh();
    Model z = fresh();
    update(z, Wheel{0, 5});
    CHECK(length(z.camera.forward - n.camera.forward) == 0.0);
}

void test_keys() {
    Model m = fresh();
    const LatLon home = center_of(m.camera);
    Effects fx = update(m, CommandIssued{1, Command::Left});
    CHECK(fx.redraw && center_of(m.camera).lon_deg > home.lon_deg);
    CHECK_NEAR(center_of(m.camera).lon_deg - home.lon_deg, 10.0, 1.0);
    update(m, CommandIssued{2, Command::Right});
    CHECK_NEAR(center_of(m.camera).lon_deg, home.lon_deg, 1e-6);
    fx = update(m, CommandIssued{3, Command::Up});
    CHECK(fx.redraw && center_of(m.camera).lat_deg < home.lat_deg);
    update(m, CommandIssued{4, Command::Down});
    CHECK_NEAR(center_of(m.camera).lat_deg, home.lat_deg, 1e-6);

    update(m, CommandIssued{5, Command::ZoomIn});
    update(m, CommandIssued{6, Command::ZoomIn});
    CHECK(m.camera.radius_px > fresh().camera.radius_px);
    update(m, CommandIssued{7, Command::Left});
    fx = update(m, CommandIssued{8, Command::Reset});
    CHECK(fx.redraw && same_camera(m.camera, fresh().camera));
    fx = update(m, CommandIssued{9, Command::Reset});
    CHECK(!fx.redraw); // already home

    CHECK(m.layers.graticule);
    fx = update(m, CommandIssued{10, Command::ToggleGraticule});
    CHECK(fx.redraw && !m.layers.graticule && m.layers.coastlines && m.layers.borders);
    update(m, CommandIssued{11, Command::ToggleBorders});
    CHECK(!m.layers.borders && m.layers.coastlines); // coastlines are not user-switchable
    update(m, CommandIssued{13, Command::ToggleGraticule});
    CHECK(m.layers.graticule);

    fx = update(m, CommandIssued{14, Command::Quit});
    CHECK(fx.quit && !fx.redraw && m.last_input_ms == 14);
    fx = update(m, CommandIssued{15, Command::Left});
    CHECK(!fx.quit);
}

void test_resize_and_expose() {
    Model m = fresh();
    const Model snapshot = m;
    Effects fx = update(m, Repaint{});
    CHECK(fx.present_only && !fx.redraw && !fx.quit && fx.wake_after_ms == 0);
    CHECK(same_camera(m.camera, snapshot.camera) && m.stage == snapshot.stage &&
          m.generation == snapshot.generation && m.dragging == snapshot.dragging);
    fx = update(m, Resize{320, 240});
    CHECK(fx.redraw && m.camera.width == 320 && m.camera.height == 240);
    CHECK_NEAR(m.camera.center_x, 160.0, 0.0);
    CHECK_NEAR(m.camera.center_y, 120.0, 0.0);
    CHECK_NEAR(m.camera.radius_px, 2.0 * 0.4 * 120.0, 1e-9); // same fraction of the window
    CHECK(length(m.camera.forward - fresh().camera.forward) == 0.0);
    fx = update(m, Resize{320, 240});
    CHECK(!fx.redraw); // same size
    const Camera before = m.camera;
    for (const Resize bad :
         {Resize{0, 10}, Resize{10, 0}, Resize{-5, 10}, Resize{5000, 10}, Resize{10, 5000}}) {
        fx = update(m, bad);
        CHECK(!fx.redraw && same_camera(m.camera, before));
    }
    // Extreme aspect: stays valid and within limits.
    update(m, Resize{4096, 1});
    CHECK(m.camera.radius_px >= min_radius_px(4096, 1) &&
          m.camera.radius_px <= max_radius_px(4096, 1));
    CHECK(basis_error(m.camera) < 1e-12);
    // Resizing keeps an in-progress drag alive and does not count as user input.
    Model d = fresh();
    update(d, PointerDown{5, 80.0, 60.0});
    update(d, Resize{200, 100});
    CHECK(d.dragging && d.last_input_ms == 5);
}

void test_styles() {
    auto m = fresh();
    CHECK(m.terrain_style == TerrainStyle::Hypsometric);
    const auto g = m.generation;
    CHECK(update(m, CommandIssued{10, Command::CycleStyle}).redraw);
    CHECK(m.terrain_style == TerrainStyle::CelShaded && m.generation > g);
    CHECK(quality_for(m).terrain_style == TerrainStyle::CelShaded);
    CHECK(toolbar_state(m)[static_cast<std::size_t>(Command::CycleStyle)].checked);
    update(m, CommandIssued{20, Command::CycleStyle});
    CHECK(m.terrain_style == TerrainStyle::Hypsometric);
    char style_text[100];
    const Msg style_command = CommandIssued{30, Command::CycleStyle};
    CHECK(format_message(style_command, style_text, sizeof(style_text)) > 0);
    auto formatted = parse_message(style_text);
    CHECK(formatted);
    if (formatted) {
        const auto* command = std::get_if<CommandIssued>(&*formatted);
        CHECK(command && command->command == Command::CycleStyle && command->time_ms == 30);
    }
    CHECK(!parse_message("style -1") && !parse_message("style 30 extra"));
    auto msg = parse_message("style 30");
    CHECK(msg);
    if (msg) {
        update(m, *msg);
        CHECK(m.terrain_style == TerrainStyle::CelShaded);
    }
}

void test_elevation_selection() {
    Model m = fresh();
    CHECK(update(m, RegisterElevation{0, 360, 180}).redraw);
    CHECK(update(m, RegisterElevation{1, 720, 360}).redraw);
    update(m, RegisterElevation{2, 180, 90});
    m.stage = QualityStage::Refine;
    m.camera.radius_px = 80;
    CHECK(quality_for(m).elevation_level == 2);
    m.camera.radius_px = 180;
    CHECK(quality_for(m).elevation_level == 0);
    m.camera.radius_px = 400;
    CHECK(quality_for(m).elevation_level == 1);
    m.camera.radius_px = 10000;
    CHECK(quality_for(m).elevation_level == 1); // finest available when all exceed target error
    const auto before = m.generation;
    CHECK(!update(m, RegisterElevation{4, 720, 360}).redraw);
    CHECK(!update(m, RegisterElevation{1, 721, 360}).redraw);
    CHECK(m.generation == before);
    char buffer[100];
    const Msg msg = RegisterElevation{3, 1080, 540};
    const auto n = format_message(msg, buffer, sizeof buffer);
    const auto back = parse_message(std::string_view(buffer, n));
    CHECK(back && std::holds_alternative<RegisterElevation>(*back));
}

void test_quality_profile_and_generation() {
    Model m = fresh();
    const auto moving = quality_for(m);
    CHECK(moving.max_chord_px == 3.0 && !moving.terrain);
    const auto generation = m.generation;
    update(m, PointerMove{1, 2, 3});
    CHECK(m.generation == generation);
    update(m, Wheel{100, 1});
    CHECK(m.generation != generation);
    update(m, Tick{250});
    CHECK(!quality_for(m).terrain);
    update(m, Tick{251});
    const auto fine = quality_for(m);
    CHECK(fine.max_chord_px == 1.5 && fine.terrain);
    const auto settled_generation = m.generation;
    update(m, Repaint{});
    CHECK(m.generation == settled_generation);
    update(m, PointerDown{300, 80, 60});
    CHECK(m.generation == settled_generation && quality_for(m).terrain); // a press is not input
    update(m, PointerMove{310, 80 + kDragSlopPx, 60});
    CHECK(m.generation == settled_generation && quality_for(m).terrain); // inside the slop
    update(m, PointerMove{320, 80 + kDragSlopPx + 1.0, 60});
    CHECK(m.generation != settled_generation && !quality_for(m).terrain);
    CHECK(quality_for(m).max_chord_px == moving.max_chord_px);
    const auto before_resize = m.generation;
    update(m, Resize{200, 100});
    CHECK(m.generation != before_resize);
}

void test_quality_wakes() {
    Model m = fresh();
    auto fx = update(m, Wheel{1000, 1});
    CHECK(m.stage == QualityStage::Moving && fx.wake_after_ms == kSettleDelayMs);
    fx = update(m, Tick{1000 + kSettleDelayMs - 1});
    CHECK(m.stage == QualityStage::Moving && fx.wake_after_ms == 1);
    fx = update(m, Tick{1000 + kSettleDelayMs});
    CHECK(m.stage == QualityStage::Settle && fx.wake_after_ms == kRefineDelayMs);
    CHECK(!fx.redraw); // no new visual detail exists yet
    fx = update(m, Tick{1000 + kSettleDelayMs + kRefineDelayMs});
    CHECK(m.stage == QualityStage::Refine && fx.wake_after_ms == 0 && !fx.redraw);
    fx = update(m, Tick{999999});
    CHECK(m.stage == QualityStage::Refine && fx.wake_after_ms == 0);
    // Every accepted input invalidates settled work, even a mode toggle with no redraw.
    fx = update(m, CommandIssued{2000, Command::ToggleDragMode});
    CHECK(m.stage == QualityStage::Moving && fx.wake_after_ms == kSettleDelayMs);
    fx = update(m, Tick{1999}); // obsolete timer from before the input
    CHECK(m.stage == QualityStage::Moving && fx.wake_after_ms == kSettleDelayMs);
    update(m, Tick{2150});
    CHECK(m.stage == QualityStage::Settle);
    // A press alone neither restarts the stage nor stops settling already under way.
    const auto generation = m.generation;
    fx = update(m, PointerDown{2151, 80.0, 60.0});
    CHECK(m.stage == QualityStage::Settle && fx.wake_after_ms == 0 && m.generation == generation);
    fx = update(m, Tick{3000});
    CHECK(m.stage == QualityStage::Refine && fx.wake_after_ms == 0);
    fx = update(m, PointerUp{3001}); // a click: no drag, so nothing at all happens
    CHECK(m.stage == QualityStage::Refine && fx.wake_after_ms == 0 && !fx.redraw &&
          m.generation == generation);
    // A real drag holds the settle clock until release, then restarts it.
    update(m, PointerDown{3100, 80.0, 60.0});
    fx = update(m, PointerMove{3110, 90.0, 60.0});
    CHECK(m.stage == QualityStage::Moving && fx.redraw && m.generation != generation);
    fx = update(m, Tick{3500});
    CHECK(m.stage == QualityStage::Moving && fx.wake_after_ms == 0); // held still
    fx = update(m, PointerUp{3501});
    CHECK(fx.wake_after_ms == kSettleDelayMs);
    update(m, Tick{3651});
    fx = update(m, Tick{3651});
    CHECK(m.stage == QualityStage::Settle && fx.wake_after_ms == kRefineDelayMs);
    fx = update(m, Tick{3652});
    CHECK(m.stage == QualityStage::Refine && fx.wake_after_ms == 0);
    update(m, PointerMove{4000, 10.0, 10.0}); // hovering is not camera input
    CHECK(m.stage == QualityStage::Refine);
    update(m, PointerDown{4000, NAN, 0.0});
    CHECK(m.stage == QualityStage::Refine);
    fx = update(m, Resize{200, 100});
    CHECK(m.stage == QualityStage::Moving && fx.wake_after_ms == kSettleDelayMs);
    // A resize has no timestamp; its first Tick establishes the new settle interval.
    fx = update(m, Tick{5000});
    CHECK(m.stage == QualityStage::Moving && fx.wake_after_ms == kSettleDelayMs);
    update(m, Tick{5150});
    update(m, Tick{5151});
    CHECK(m.stage == QualityStage::Refine);
    fx = update(m, Repaint{});
    CHECK(m.stage == QualityStage::Refine && fx.wake_after_ms == 0);
}

// A click without dragging changes nothing: same model, no effects, settled stage kept.
void test_click_changes_nothing() {
    Model m = fresh();
    update(m, Tick{0});
    update(m, Tick{150});
    update(m, Tick{151});
    CHECK(m.stage == QualityStage::Refine);
    const Model before = m;
    Effects fx = update(m, PointerDown{1000, 70.0, 60.0});
    CHECK(!fx.redraw && !fx.present_only && !fx.quit && fx.wake_after_ms == 0);
    CHECK(m.stage == before.stage && m.generation == before.generation &&
          m.stage_since_ms == before.stage_since_ms && same_camera(m.camera, before.camera));
    CHECK(m.dragging && !m.drag_active && !m.drag_moved);
    // Jitter inside the slop in every direction still changes nothing.
    for (const ScreenPoint to : {ScreenPoint{70.0 + kDragSlopPx, 60.0},
                                 {70.0 - kDragSlopPx, 60.0},
                                 {70.0, 60.0 + kDragSlopPx},
                                 {70.0, 60.0 - kDragSlopPx}}) {
        fx = update(m, PointerMove{1010, to.x, to.y});
        CHECK(!fx.redraw && fx.wake_after_ms == 0 && !m.drag_active && !m.drag_moved);
        CHECK(m.stage == before.stage && m.generation == before.generation &&
              same_camera(m.camera, before.camera));
    }
    CHECK(effective_layers(m).graticule);
    fx = update(m, PointerUp{1020});
    CHECK(!fx.redraw && fx.wake_after_ms == 0 && !m.dragging && !m.drag_active);
    CHECK(m.stage == before.stage && m.generation == before.generation &&
          same_camera(m.camera, before.camera));
    // The release of a click still ends the press (platforms pick on dragging && !drag_moved).
    // One pixel past the slop is a drag: the globe follows from the press position, not the
    // point where the slop was crossed.
    m.drag_mode = DragMode::Trackball; // exact grab-the-point behavior
    update(m, PointerDown{2000, 70.0, 60.0});
    fx = update(m, PointerMove{2010, 70.0 + kDragSlopPx + 1.0, 60.0});
    CHECK(m.drag_active && m.drag_moved && fx.redraw && fx.wake_after_ms == kSettleDelayMs);
    CHECK(m.stage == QualityStage::Moving && m.generation != before.generation);
    CHECK(!effective_layers(m).graticule);
    CHECK(same_camera(m.camera,
                      drag_rotate(before.camera, {70.0, 60.0}, {70.0 + kDragSlopPx + 1.0, 60.0})));
    fx = update(m, PointerUp{2020});
    CHECK(fx.redraw && fx.wake_after_ms == kSettleDelayMs && !m.dragging);
    // A double click that changes nothing (already at the maximum zoom) is not input either.
    Model z = fresh();
    z.camera = *make_camera({{0.0, 0.0}, 0.0, max_radius_px(160, 120), 160, 120});
    update(z, Tick{0});
    update(z, Tick{150});
    update(z, Tick{151});
    CHECK(z.stage == QualityStage::Refine);
    const auto zoom_generation = z.generation;
    update(z, PointerDown{5000, 80.0, 60.0});
    update(z, PointerUp{5010});
    fx = update(z, PointerDown{5100, 80.0, 60.0});
    CHECK(z.stage == QualityStage::Refine && z.generation == zoom_generation && !fx.redraw);
}

void test_stage_trace_replay() {
    const char* lines[] = {
        "wheel 1000 1", "tick 1149", "tick 1150", "tick 1151", "key 1200 toggle_borders",
        "tick 1201",    "tick 1350", "tick 1351"};
    const QualityStage stages[] = {QualityStage::Moving, QualityStage::Moving, QualityStage::Settle,
                                   QualityStage::Refine, QualityStage::Moving, QualityStage::Moving,
                                   QualityStage::Settle, QualityStage::Refine};
    Model a = fresh();
    Model b = fresh();
    for (std::size_t i = 0; i < sizeof(lines) / sizeof(lines[0]); ++i) {
        const auto msg = parse_message(lines[i]);
        CHECK(msg.has_value());
        if (!msg) {
            continue;
        }
        const Effects fa = update(a, *msg);
        char buf[200];
        const auto n = format_message(*msg, buf, sizeof buf);
        const auto back = parse_message(std::string_view(buf, n));
        CHECK(back.has_value());
        if (!back) {
            continue;
        }
        const Effects fb = update(b, *back);
        CHECK(a.stage == stages[i] && b.stage == stages[i]);
        CHECK(fa.redraw == fb.redraw && fa.wake_after_ms == fb.wake_after_ms);
        CHECK(same_camera(a.camera, b.camera));
    }
}

void test_toolbar_state() {
    Model m = fresh();
    const auto state = [](const Model& model, Command command) {
        return toolbar_state(model)[static_cast<std::size_t>(command)];
    };
    const auto initial = toolbar_state(m);
    CHECK(initial.size() == kCommandCount);
    for (std::size_t i = 0; i < initial.size(); ++i) {
        CHECK(initial[i].visible);
        CHECK(initial[i].enabled == (i != static_cast<std::size_t>(Command::AlignNorth)));
        CHECK(initial[i].checked == (i == static_cast<std::size_t>(Command::ToggleGraticule) ||
                                     i == static_cast<std::size_t>(Command::ToggleBorders)));
    }
    const Command toggles[] = {Command::ToggleGraticule, Command::ToggleBorders,
                               Command::ToggleDragMode};
    for (Command command : toggles) {
        const bool before = state(m, command).checked;
        update(m, CommandIssued{1, command});
        CHECK(state(m, command).checked != before);
        update(m, CommandIssued{2, command});
        CHECK(state(m, command).checked == before);
    }
    m.dragging = true;
    CHECK(state(m, Command::ToggleGraticule).checked); // preference, even while hidden
    for (double heading : {30.0, 180.0, -90.0}) {
        m.camera = *make_camera({{20.0, 40.0}, heading, 48.0, 160, 120});
        const Camera before = m.camera;
        CHECK(state(m, Command::AlignNorth).enabled);
        CHECK(same_camera(before, m.camera)); // querying is pure
        update(m, CommandIssued{3, Command::AlignNorth});
        CHECK(!state(m, Command::AlignNorth).enabled);
    }
    m.camera.radius_px = min_radius_px(160, 120);
    CHECK(!state(m, Command::ZoomOut).enabled && state(m, Command::ZoomIn).enabled);
    m.camera.radius_px = max_radius_px(160, 120);
    CHECK(!state(m, Command::ZoomIn).enabled && state(m, Command::ZoomOut).enabled);
}

void test_refinement_focus() {
    auto m = initial_model(200, 150);
    CHECK(quality_for(m).focus.x == m.camera.center_x);
    CHECK(quality_for(m).focus.y == m.camera.center_y);
    // Touches never move the focus: refinement always radiates from the view center.
    update(m, PointerDown{100, 70, 60});
    update(m, PointerMove{100, 100, 80});
    update(m, PointerUp{101});
    CHECK(quality_for(m).focus.x == m.camera.center_x);
    CHECK(quality_for(m).focus.y == m.camera.center_y);
    for (const char* order : {"rectangle", "circle", "globe"}) {
        auto msg = parse_message(std::string("order 3000 ") + order);
        CHECK(msg && std::holds_alternative<SetRefinementOrder>(*msg));
        if (!msg)
            continue;
        auto generation = m.generation;
        CHECK(update(m, *msg).redraw && m.generation > generation);
        CHECK(quality_for(m).order == std::get<SetRefinementOrder>(*msg).order);
        char text[100];
        auto n = format_message(*msg, text, sizeof text);
        CHECK(std::string(text, n) == std::string("order 3000 ") + order);
    }
    CHECK(!parse_message("order 1 random"));
    // The retired names and the brief-lived "square" are unknown.
    for (const char* old : {"rows", "center", "dissolve", "square"})
        CHECK(!parse_message(std::string("order 1 ") + old));
    update(m, Resize{300, 200});
    CHECK(quality_for(m).focus.x == m.camera.center_x);
}

void test_trace_text_round_trip() {
    const Msg msgs[] = {
        PointerDown{12, 1.0 / 3.0, -2.5e-7},
        PointerMove{99999999, 123456.789012345678, -0.0},
        PointerUp{0},
        Wheel{7, -3},
        Wheel{8, 1000000},
        CommandIssued{9, Command::ToggleBorders},
        CommandIssued{10, Command::Quit},
        Resize{640, 480},
        Repaint{},
        Tick{12345},
    };
    for (const Msg& msg : msgs) {
        char buf[200];
        const std::size_t n = format_message(msg, buf, sizeof buf);
        CHECK(n > 0 && n < sizeof buf);
        const auto back = parse_message(std::string_view(buf, n));
        CHECK(back.has_value());
        char buf2[200];
        const std::size_t n2 = back ? format_message(*back, buf2, sizeof buf2) : 0;
        CHECK(n2 == n && std::memcmp(buf, buf2, n) == 0);
        if (const auto* d = std::get_if<PointerDown>(&msg)) { // exact double round trip
            const auto* e = back ? std::get_if<PointerDown>(&*back) : nullptr;
            CHECK(e != nullptr && e->x == d->x && e->y == d->y && e->time_ms == d->time_ms);
        }
    }
    char tiny[4];
    CHECK(format_message(Repaint{}, tiny, sizeof tiny) ==
          0); // does not fit: reported, not truncated
    CHECK(format_message(Repaint{}, nullptr, 10) == 0);
    for (int k = 0; k <= static_cast<int>(Command::Quit); ++k) {
        const std::string line = std::string("key 5 ") + command_name(static_cast<Command>(k));
        const auto back = parse_message(line);
        const auto* kp = back ? std::get_if<CommandIssued>(&*back) : nullptr;
        CHECK(kp != nullptr && kp->command == static_cast<Command>(k));
    }
}

void test_trace_parse_rejects_malformed() {
    const char* bad[] = {
        "elevation 4 360 180",
        "elevation 1 361 180",
        "elevation 1 0 0",
        "elevation 1 32768 16384",
        "elevation 1 360 180 extra",
        "",
        "   ",
        "# comment",
        "down",
        "down 1 2",
        "down 1 2 3 4",
        "down x 2 3",
        "down -1 2 3",
        "down 1 nan 3",
        "down 1 2 inf",
        "move 1 2e999 3",
        "move 1 2 3abc",
        "tick",
        "tick -1",
        "tick 1 2",
        "tick +1",
        "up",
        "up 1 2",
        "up -4",
        "wheel 1",
        "wheel 1 2.5",
        "wheel 1 99999999",
        "key 1",
        "key 1 nosuchkey",
        "key 1 left right",
        "resize 0 5",
        "resize 5",
        "resize 5000 5",
        "resize a b",
        "expose now",
        "EXPOSE",
        "jump 1 2 3",
        "down 99999999999999999999 1 2",
        "down 1 2 3 4 5 6 7",
        "down -0 1 2",
        "up +5",
        "up 5z",
        "down 1x 2 3",
        "wheel 1x 2",
        "wheel 1 +",
    };
    for (const char* line : bad) {
        if (parse_message(line).has_value()) {
            std::printf("accepted: \"%s\"\n", line);
            CHECK(false);
        }
    }
    CHECK(!parse_message(std::string(300, 'a')).has_value());
    CHECK(!parse_message(std::string_view("down 1 2\0 3", 11)).has_value()); // embedded NUL
    // Tabs, extra spaces and CR are tolerated.
    CHECK(parse_message("  down\t1   2 3\r").has_value());
}

std::uint64_t fnv1a(const Framebuffer& fb) {
    std::uint64_t h = 1469598103934665603ULL;
    for (int y = 0; y < fb.height(); ++y) {
        for (int x = 0; x < fb.width(); ++x) {
            const Rgb565 p = fb.pixel(x, y);
            h = (h ^ (p & 0xFFU)) * 1099511628211ULL;
            h = (h ^ (p >> 8)) * 1099511628211ULL;
        }
    }
    return h;
}

bool read_file(const std::string& path, std::string& out) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (f == nullptr) {
        return false;
    }
    char buf[4096];
    std::size_t n = 0;
    out.clear();
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) {
        out.append(buf, n);
    }
    std::fclose(f);
    return true;
}

struct Replay {
    Model model;
    int messages = 0;
    int redraws = 0;
    bool quit = false;
    std::vector<Camera> cameras; // after every message
};

Replay replay(const std::string& text) {
    Replay r;
    r.model = fresh();
    std::size_t pos = 0;
    while (pos < text.size()) {
        std::size_t end = text.find('\n', pos);
        if (end == std::string::npos) {
            end = text.size();
        }
        const auto msg = parse_message(std::string_view(text).substr(pos, end - pos));
        pos = end + 1;
        if (!msg) {
            continue;
        }
        const Effects fx = update(r.model, *msg);
        ++r.messages;
        r.redraws += fx.redraw ? 1 : 0;
        r.quit |= fx.quit;
        r.cameras.push_back(r.model.camera);
        // Invariants after every single message.
        CHECK(basis_error(r.model.camera) < 1e-12);
        CHECK(r.model.camera.radius_px >=
                  min_radius_px(r.model.camera.width, r.model.camera.height) - 1e-9 &&
              r.model.camera.radius_px <=
                  max_radius_px(r.model.camera.width, r.model.camera.height) + 1e-9);
        CHECK(r.model.camera.center_x == r.model.camera.width / 2.0);
    }
    return r;
}

void test_replay(const std::string& root) {
    std::string trace;
    CHECK(read_file(root + "/tests/data/trace1.txt", trace));
    const Replay a = replay(trace);
    const Replay b = replay(trace);
    CHECK(a.messages > 100);
    // Deterministic: the whole path through camera space is identical, message by message.
    CHECK(a.messages == b.messages && a.redraws == b.redraws &&
          a.cameras.size() == b.cameras.size());
    for (std::size_t i = 0; i < a.cameras.size() && i < b.cameras.size(); ++i) {
        if (!same_camera(a.cameras[i], b.cameras[i])) {
            std::printf("replays diverge at message %zu\n", i);
            CHECK(false);
            break;
        }
    }
    CHECK(!a.quit && !a.model.dragging);
    CHECK(a.model.camera.width == 200 && a.model.camera.height == 150);
    CHECK(!a.model.layers.borders && a.model.layers.graticule);
    CHECK(a.redraws > 90 && a.redraws < a.messages);

    // And the pixels are identical: render the final state with the real atlas.
    std::string atlas_text;
    CHECK(read_file(root + "/data/atlas.gatl", atlas_text));
    const auto atlas =
        Atlas::parse(reinterpret_cast<const std::uint8_t*>(atlas_text.data()), atlas_text.size());
    CHECK(atlas.has_value());
    if (!atlas) {
        return;
    }
    auto fa = Framebuffer::create(200, 150);
    auto fb = Framebuffer::create(200, 150);
    CHECK(fa && fb);
    if (!fa || !fb) {
        return;
    }
    render_globe(*fa, a.model.camera, *atlas, kDefaultPalette, a.model.layers);
    render_globe(*fb, b.model.camera, *atlas, kDefaultPalette, b.model.layers);
    CHECK(std::memcmp(fa->data(), fb->data(), fa->byte_size()) == 0);
    std::printf("final frame hash %016llx\n", static_cast<unsigned long long>(fnv1a(*fa)));
    CHECK(fnv1a(*fa) == 0x9b4a458633a32d9dULL);
}

} // namespace

int main(int argc, char** argv) {
    const std::string root = argc > 1 ? argv[1] : ".";
    test_refinement_focus();
    test_initial_model();
    test_drag();
    test_turntable_drag();
    test_align_north_and_mode_toggle();
    test_double_click_keeps_north_up_in_turntable();
    test_grid_hidden_while_dragging();
    test_click_changes_nothing();
    test_double_click();
    test_zoom_and_limits();
    test_keys();
    test_resize_and_expose();
    test_styles();
    test_elevation_selection();
    test_quality_profile_and_generation();
    test_quality_wakes();
    test_stage_trace_replay();
    test_toolbar_state();
    test_trace_text_round_trip();
    test_trace_parse_rejects_malformed();
    test_replay(root);
    return TEST_MAIN_RESULT();
}
