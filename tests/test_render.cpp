// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Usage: test_render [REPO_ROOT] [--update-golden]
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "atlas_util.hpp"
#include "check.hpp"
#include "globe/raster.hpp"
#include "globe/render.hpp"

using namespace globe;
using namespace atlas_util;

namespace {

bool g_update_golden = false;

struct Lcg {
    std::uint32_t state;
    double next() { // [0, 1)
        state = state * 1664525U + 1013904223U;
        return static_cast<double>(state >> 8) / 16777216.0;
    }
};

Framebuffer make_fb(int w, int h) {
    auto fb = Framebuffer::create(w, h);
    CHECK(fb.has_value());
    return std::move(*fb);
}

std::vector<std::pair<int, int>> pixels_set(const Framebuffer& fb, Rgb565 color) {
    std::vector<std::pair<int, int>> out;
    for (int y = 0; y < fb.height(); ++y) {
        for (int x = 0; x < fb.width(); ++x) {
            if (fb.pixel(x, y) == color) {
                out.emplace_back(x, y);
            }
        }
    }
    return out;
}

std::size_t count(const Framebuffer& fb, Rgb565 color) {
    return pixels_set(fb, color).size();
}

constexpr Rgb565 kInk = 0xFFFF;

void test_line_exact_pixels() {
    Framebuffer fb = make_fb(8, 8);
    fb.clear(0);
    draw_line(fb, 0.5, 0.5, 5.5, 2.5, kInk); // pixels (0,0) to (5,2)
    const std::vector<std::pair<int, int>> expected = {{0, 0}, {1, 0}, {2, 1},
                                                       {3, 1}, {4, 2}, {5, 2}};
    // pixels_set scans rows; compare as sets.
    auto got = pixels_set(fb, kInk);
    CHECK(got.size() == expected.size());
    for (const auto& p : expected) {
        CHECK(fb.pixel(p.first, p.second) == kInk);
    }

    fb.clear(0);
    draw_line(fb, 0, 0, 7, 7, kInk);
    for (int i = 0; i < 8; ++i) {
        CHECK(fb.pixel(i, i) == kInk);
    }
    CHECK(count(fb, kInk) == 8);

    fb.clear(0);
    draw_line(fb, 1, 3, 6, 3, kInk); // horizontal
    CHECK(count(fb, kInk) == 6 && fb.pixel(1, 3) == kInk && fb.pixel(6, 3) == kInk);
    fb.clear(0);
    draw_line(fb, 4, 7, 4, 1, kInk); // vertical, upward
    CHECK(count(fb, kInk) == 7 && fb.pixel(4, 1) == kInk && fb.pixel(4, 7) == kInk);
    fb.clear(0);
    draw_line(fb, 3.2, 3.9, 3.7, 3.1, kInk); // both ends in the same pixel
    CHECK(count(fb, kInk) == 1 && fb.pixel(3, 3) == kInk);
    fb.clear(0);
    draw_line(fb, 0.5, 6.5, 2.5, 0.5, kInk); // steep, upward
    CHECK(fb.pixel(0, 6) == kInk && fb.pixel(2, 0) == kInk);
    CHECK(count(fb, kInk) == 7); // one pixel per row
}

void test_line_symmetry_and_connectivity() {
    Lcg rng{5};
    Framebuffer a = make_fb(40, 40);
    Framebuffer b = make_fb(40, 40);
    for (int i = 0; i < 500; ++i) {
        const double x0 = rng.next() * 60.0 - 10.0;
        const double y0 = rng.next() * 60.0 - 10.0;
        const double x1 = rng.next() * 60.0 - 10.0;
        const double y1 = rng.next() * 60.0 - 10.0;
        a.clear(0);
        b.clear(0);
        draw_line(a, x0, y0, x1, y1, kInk);
        draw_line(b, x1, y1, x0, y0, kInk);
        CHECK(std::memcmp(a.data(), b.data(), a.byte_size()) == 0);
    }
    // A line wholly inside has exactly max(|dx|, |dy|) + 1 pixels (one per major step) and every
    // pixel touches the next (8-connected).
    for (int i = 0; i < 300; ++i) {
        const int x0 = static_cast<int>(rng.next() * 40);
        const int y0 = static_cast<int>(rng.next() * 40);
        const int x1 = static_cast<int>(rng.next() * 40);
        const int y1 = static_cast<int>(rng.next() * 40);
        a.clear(0);
        draw_line(a, x0 + 0.5, y0 + 0.5, x1 + 0.5, y1 + 0.5, kInk);
        const int major = std::max(std::abs(x1 - x0), std::abs(y1 - y0));
        CHECK(static_cast<int>(count(a, kInk)) == major + 1);
        CHECK(a.pixel(x0, y0) == kInk && a.pixel(x1, y1) == kInk);
    }
}

// Clipping is exact: drawing into a small buffer equals cropping the same line drawn into a
// larger one.
void test_line_clipping_is_exact() {
    Lcg rng{11};
    const int ox = 70;
    const int oy = 50;
    Framebuffer big = make_fb(200, 200);
    Framebuffer small = make_fb(40, 30);
    for (int i = 0; i < 2000; ++i) {
        const double x0 = rng.next() * 190.0 + 5.0;
        const double y0 = rng.next() * 190.0 + 5.0;
        const double x1 = rng.next() * 190.0 + 5.0;
        const double y1 = rng.next() * 190.0 + 5.0;
        big.clear(0);
        small.clear(0);
        draw_line(big, x0, y0, x1, y1, kInk);
        draw_line(small, x0 - ox, y0 - oy, x1 - ox, y1 - oy, kInk);
        for (int y = 0; y < small.height(); ++y) {
            for (int x = 0; x < small.width(); ++x) {
                if (small.pixel(x, y) != big.pixel(x + ox, y + oy)) {
                    std::printf("mismatch line %d (%f,%f)-(%f,%f) at %d,%d\n", i, x0, y0, x1, y1, x,
                                y);
                    CHECK(false);
                    return;
                }
            }
        }
    }
}

void test_line_extremes() {
    Framebuffer fb = make_fb(16, 8);
    fb.clear(0);
    const double nan = std::nan("");
    const double inf = INFINITY;
    draw_line(fb, nan, 0, 5, 5, kInk);
    draw_line(fb, 0, 0, inf, 5, kInk);
    draw_line(fb, -inf, 0, 5, 5, kInk);
    CHECK(count(fb, kInk) == 0);
    // Absurdly long lines still draw their visible part, in bounded time.
    draw_line(fb, -1e9, 5.5, 1e9, 5.5, kInk);
    CHECK(count(fb, kInk) == 16);
    fb.clear(0);
    draw_line(fb, -1e9, -1e9, 1e9, 1e9, kInk); // diagonal through the origin
    CHECK(fb.pixel(3, 3) == kInk && fb.pixel(0, 0) == kInk && fb.pixel(7, 7) == kInk);
    // Beyond double precision the +-1e6 clip box cannot be resolved; the result is unspecified
    // but must be safe.
    draw_line(fb, 1e300, 1e300, -1e300, -1e300, kInk);
    fb.clear(0);
    draw_line(fb, -50, -50, -10, -10, kInk); // entirely outside
    draw_line(fb, 20, 0, 90, 4, kInk);       // entirely right of the buffer
    CHECK(count(fb, kInk) == 0);
}

void test_fill_disc() {
    Lcg rng{23};
    Framebuffer fb = make_fb(48, 40);
    for (int i = 0; i < 300; ++i) {
        const double cx = rng.next() * 70.0 - 10.0;
        const double cy = rng.next() * 60.0 - 10.0;
        const double r = rng.next() * 30.0 + 0.2;
        fb.clear(0);
        fill_disc(fb, cx, cy, r, kInk);
        for (int y = 0; y < fb.height(); ++y) {
            for (int x = 0; x < fb.width(); ++x) {
                const double dx = x + 0.5 - cx;
                const double dy = y + 0.5 - cy;
                const bool inside = dx * dx + dy * dy <= r * r;
                if ((fb.pixel(x, y) == kInk) != inside) {
                    std::printf("disc mismatch c=(%f,%f) r=%f at %d,%d\n", cx, cy, r, x, y);
                    CHECK(false);
                    return;
                }
            }
        }
    }
    fb.clear(0);
    fill_disc(fb, 24, 20, 1e12, kInk); // covers everything, still fast
    CHECK(count(fb, kInk) == 48U * 40U);
    fb.clear(0);
    fill_disc(fb, 24, 20, 0.0, kInk);
    fill_disc(fb, 24, 20, -3.0, kInk);
    fill_disc(fb, std::nan(""), 20, 5.0, kInk);
    fill_disc(fb, 24, 20, INFINITY, kInk);
    CHECK(count(fb, kInk) == 0);
}

// ---- globe rendering ------------------------------------------------------------------

constexpr Palette kTestPalette = {
    0x0000, // background
    0x001F, // ocean (blue)
    0x07E0, // graticule (green)
    0xFFE0, // coastline (yellow)
    0xF800, // border (red)
};

char glyph(Rgb565 c) {
    if (c == kTestPalette.background)
        return '.';
    if (c == kTestPalette.ocean)
        return '~';
    if (c == kTestPalette.graticule)
        return '+';
    if (c == kTestPalette.coastline)
        return '#';
    if (c == kTestPalette.border)
        return 'x';
    return '?';
}

std::string ascii(const Framebuffer& fb) {
    std::string out;
    for (int y = 0; y < fb.height(); ++y) {
        for (int x = 0; x < fb.width(); ++x) {
            out += glyph(fb.pixel(x, y));
        }
        out += '\n';
    }
    return out;
}

std::uint16_t lon_q(double lon) {
    return static_cast<std::uint16_t>((lon + 180.0) / 360.0 * 65536.0);
}
std::uint16_t lat_q(double lat) {
    return static_cast<std::uint16_t>((lat + 90.0) / 360.0 * 65536.0);
}

// A hand-made world: the equator from -150 to 150 (coast), the lon 0 meridian from -80 to 80
// (border), and a small coastline on the far side at lon 180 (hidden when facing lon 0).
Bytes synthetic_atlas() {
    Poly equator{1, {}};
    for (int lon = -150; lon <= 150; lon += 30)
        equator.points.emplace_back(lon_q(lon), lat_q(0));
    Poly meridian{2, {}};
    for (int lat = -80; lat <= 80; lat += 20)
        meridian.points.emplace_back(lon_q(0), lat_q(lat));
    Poly rear{1, {{0, lat_q(0)}, {0, lat_q(20)}, {0, lat_q(40)}}};
    return make_atlas({equator, meridian, rear}, "synthetic");
}

Atlas parse_or_die(const Bytes& b) {
    auto a = Atlas::parse(b.data(), b.size());
    CHECK(a.has_value());
    if (!a) {
        std::abort();
    }
    return *a;
}

Camera cam(double lat, double lon, double heading, double radius, int w, int h) {
    auto c = make_camera({{lat, lon}, heading, radius, w, h});
    CHECK(c.has_value());
    return c.value_or(Camera{});
}

void check_golden(const std::string& root, const char* name, const Framebuffer& fb) {
    const std::string path = root + "/tests/golden/" + name + ".txt";
    const std::string got = ascii(fb);
    if (g_update_golden) {
        std::FILE* f = std::fopen(path.c_str(), "wb");
        CHECK(f != nullptr);
        if (f) {
            std::fwrite(got.data(), 1, got.size(), f);
            std::fclose(f);
        }
        return;
    }
    std::FILE* f = std::fopen(path.c_str(), "rb");
    CHECK(f != nullptr);
    if (!f) {
        return;
    }
    std::string want;
    char buf[1024];
    std::size_t n = 0;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0)
        want.append(buf, n);
    std::fclose(f);
    if (want != got) {
        std::printf("golden frame %s differs.\n--- want\n%s--- got\n%s", name, want.c_str(),
                    got.c_str());
    }
    CHECK(want == got);
}

void test_golden_frames(const std::string& root) {
    const Bytes bytes = synthetic_atlas();
    const Atlas atlas = parse_or_die(bytes);
    Framebuffer fb = make_fb(48, 36);
    render_globe(fb, cam(0, 0, 0, 16, 48, 36), atlas, kTestPalette, Layers{});
    check_golden(root, "front", fb);
    Framebuffer fb2 = make_fb(40, 40);
    render_globe(fb2, cam(60, 120, 30, 14, 40, 40), atlas, kTestPalette, Layers{});
    check_golden(root, "oblique", fb2);
}

void test_render_properties() {
    const Bytes bytes = synthetic_atlas();
    const Atlas atlas = parse_or_die(bytes);
    Framebuffer fb = make_fb(96, 72);

    // Nothing is drawn outside the globe's disc (one pixel of rounding allowance).
    const Camera front = cam(0, 0, 0, 30, 96, 72);
    render_globe(fb, front, atlas, kTestPalette, Layers{});
    for (int y = 0; y < fb.height(); ++y) {
        for (int x = 0; x < fb.width(); ++x) {
            const double d = std::hypot(x + 0.5 - front.center_x, y + 0.5 - front.center_y);
            if (d > front.radius_px + 1.5) {
                CHECK(fb.pixel(x, y) == kTestPalette.background);
            }
        }
    }
    CHECK(count(fb, kTestPalette.ocean) > 0);
    CHECK(count(fb, kTestPalette.coastline) > 0);
    CHECK(count(fb, kTestPalette.border) > 0);
    CHECK(count(fb, kTestPalette.graticule) > 0);

    // Layer switches.
    render_globe(fb, front, atlas, kTestPalette, Layers{false, true, true});
    CHECK(count(fb, kTestPalette.graticule) == 0 && count(fb, kTestPalette.coastline) > 0);
    render_globe(fb, front, atlas, kTestPalette, Layers{true, false, true});
    CHECK(count(fb, kTestPalette.coastline) == 0 && count(fb, kTestPalette.border) > 0);
    render_globe(fb, front, atlas, kTestPalette, Layers{true, true, false});
    CHECK(count(fb, kTestPalette.border) == 0 && count(fb, kTestPalette.coastline) > 0);
    render_globe(fb, front, atlas, kTestPalette, Layers{false, false, false});
    CHECK(count(fb, kTestPalette.ocean) > 0 && count(fb, kTestPalette.coastline) == 0 &&
          count(fb, kTestPalette.border) == 0 && count(fb, kTestPalette.graticule) == 0);

    // The far-side feature (lon 180, lat 0..40) appears only when that side faces the camera;
    // with only coastlines on and the front view, the coast pixels are the equator alone, so
    // nothing lies above the equator line.
    render_globe(fb, front, atlas, kTestPalette, Layers{false, true, false});
    for (int y = 0; y < fb.height(); ++y) {
        for (int x = 0; x < fb.width(); ++x) {
            if (fb.pixel(x, y) == kTestPalette.coastline) {
                CHECK(std::abs(y - 36) <= 1);
            }
        }
    }
    const Camera back = cam(20, 180, 0, 30, 96, 72);
    render_globe(fb, back, atlas, kTestPalette, Layers{false, true, false});
    bool above_equator = false;
    for (int y = 0; y < 34; ++y) {
        for (int x = 0; x < fb.width(); ++x) {
            above_equator |= fb.pixel(x, y) == kTestPalette.coastline;
        }
    }
    CHECK(above_equator);

    // Determinism: same inputs, same bytes.
    Framebuffer a = make_fb(96, 72);
    Framebuffer b = make_fb(96, 72);
    const RenderStats sa = render_globe(a, front, atlas, kTestPalette, Layers{});
    const RenderStats sb = render_globe(b, front, atlas, kTestPalette, Layers{});
    CHECK(std::memcmp(a.data(), b.data(), a.byte_size()) == 0);
    CHECK(sa.segments == sb.segments && sa.lines_drawn == sb.lines_drawn);
    CHECK(sa.segments_visible > 0 && sa.segments_visible < sa.segments);
}

void test_extreme_cameras(const std::string& root) {
    // Many random and extreme cameras over the real atlas: no crash, no sanitizer finding.
    std::FILE* f = std::fopen((root + "/data/atlas.gatl").c_str(), "rb");
    CHECK(f != nullptr);
    if (!f) {
        return;
    }
    Bytes bytes;
    std::uint8_t buf[4096];
    std::size_t n = 0;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0)
        bytes.insert(bytes.end(), buf, buf + n);
    std::fclose(f);
    const Atlas atlas = parse_or_die(bytes);

    Framebuffer fb = make_fb(64, 48);
    Lcg rng{77};
    const double radii[] = {1e-3, 0.5, 5.0, 24.0, 300.0, 1e4};
    for (int i = 0; i < 48; ++i) {
        const Camera c = cam(rng.next() * 180 - 90, rng.next() * 720 - 360, rng.next() * 360,
                             radii[i % 6], 64, 48);
        render_globe(fb, c, atlas, kDefaultPalette, Layers{});
    }
    // Absurd zoom: each segment hits the subdivision cap; once is enough to prove it is safe.
    render_globe(fb, cam(10, 20, 0, 1e6, 64, 48), atlas, kDefaultPalette, Layers{});
    render_globe(fb, cam(-80, 170, 45, 1e9, 64, 48), atlas, kDefaultPalette, Layers{});

    // A normal view: every atlas segment (7653) and graticule segment (1320) is considered.
    const Camera normal = cam(20, 10, 0, 22, 64, 48);
    const RenderStats s = render_globe(fb, normal, atlas, kDefaultPalette, Layers{});
    CHECK(s.segments == 7653U + 1320U);
    CHECK(s.segments_visible > 2000 && s.segments_visible < 7000);
    CHECK(s.lines_drawn >= s.segments_visible);
    CHECK(count(fb, kDefaultPalette.coastline) > 50);
    CHECK(count(fb, kDefaultPalette.border) > 20);
}

} // namespace

int main(int argc, char** argv) {
    std::string root = ".";
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--update-golden") == 0) {
            g_update_golden = true;
        } else {
            root = argv[i];
        }
    }
    test_line_exact_pixels();
    test_line_symmetry_and_connectivity();
    test_line_clipping_is_exact();
    test_line_extremes();
    test_fill_disc();
    test_golden_frames(root);
    test_render_properties();
    test_extreme_cameras(root);
    return TEST_MAIN_RESULT();
}
