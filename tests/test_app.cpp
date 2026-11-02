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
    CHECK(!fx.redraw && !m.dragging && m.last_input_ms == 60);
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
    Effects fx = update(m, KeyPress{1, Key::Left});
    CHECK(fx.redraw && center_of(m.camera).lon_deg > home.lon_deg);
    CHECK_NEAR(center_of(m.camera).lon_deg - home.lon_deg, 10.0, 1.0);
    update(m, KeyPress{2, Key::Right});
    CHECK_NEAR(center_of(m.camera).lon_deg, home.lon_deg, 1e-6);
    fx = update(m, KeyPress{3, Key::Up});
    CHECK(fx.redraw && center_of(m.camera).lat_deg < home.lat_deg);
    update(m, KeyPress{4, Key::Down});
    CHECK_NEAR(center_of(m.camera).lat_deg, home.lat_deg, 1e-6);

    update(m, KeyPress{5, Key::ZoomIn});
    update(m, KeyPress{6, Key::ZoomIn});
    CHECK(m.camera.radius_px > fresh().camera.radius_px);
    update(m, KeyPress{7, Key::Left});
    fx = update(m, KeyPress{8, Key::Reset});
    CHECK(fx.redraw && same_camera(m.camera, fresh().camera));
    fx = update(m, KeyPress{9, Key::Reset});
    CHECK(!fx.redraw); // already home

    CHECK(m.layers.graticule);
    fx = update(m, KeyPress{10, Key::ToggleGraticule});
    CHECK(fx.redraw && !m.layers.graticule && m.layers.coastlines && m.layers.borders);
    update(m, KeyPress{11, Key::ToggleBorders});
    update(m, KeyPress{12, Key::ToggleCoastlines});
    CHECK(!m.layers.borders && !m.layers.coastlines);
    update(m, KeyPress{13, Key::ToggleGraticule});
    CHECK(m.layers.graticule);

    fx = update(m, KeyPress{14, Key::Quit});
    CHECK(fx.quit && !fx.redraw && m.last_input_ms == 14);
    fx = update(m, KeyPress{15, Key::Left});
    CHECK(!fx.quit);
}

void test_resize_and_expose() {
    Model m = fresh();
    Effects fx = update(m, Expose{});
    CHECK(fx.redraw && !fx.quit);
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

void test_trace_text_round_trip() {
    const Msg msgs[] = {
        PointerDown{12, 1.0 / 3.0, -2.5e-7},
        PointerMove{99999999, 123456.789012345678, -0.0},
        PointerUp{0},
        Wheel{7, -3},
        Wheel{8, 1000000},
        KeyPress{9, Key::ToggleCoastlines},
        KeyPress{10, Key::Quit},
        Resize{640, 480},
        Expose{},
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
    CHECK(format_message(Expose{}, tiny, sizeof tiny) ==
          0); // does not fit: reported, not truncated
    CHECK(format_message(Expose{}, nullptr, 10) == 0);
    for (int k = 0; k <= static_cast<int>(Key::Quit); ++k) {
        const std::string line = std::string("key 5 ") + key_name(static_cast<Key>(k));
        const auto back = parse_message(line);
        const auto* kp = back ? std::get_if<KeyPress>(&*back) : nullptr;
        CHECK(kp != nullptr && kp->key == static_cast<Key>(k));
    }
}

void test_trace_parse_rejects_malformed() {
    const char* bad[] = {
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
    CHECK(fnv1a(*fa) == 0xb3519ed59541d2b0ULL);
}

} // namespace

int main(int argc, char** argv) {
    const std::string root = argc > 1 ? argv[1] : ".";
    test_initial_model();
    test_drag();
    test_zoom_and_limits();
    test_keys();
    test_resize_and_expose();
    test_trace_text_round_trip();
    test_trace_parse_rejects_malformed();
    test_replay(root);
    return TEST_MAIN_RESULT();
}
