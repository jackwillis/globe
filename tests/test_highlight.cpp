// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/highlight.hpp"
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
int main(int argc, char** argv) {
    const std::string root = argc > 1 ? argv[1] : ".";
    auto bytes = read(root + "/tests/data/countries.gpol");
    auto data = Countries::parse(bytes.data(), bytes.size());
    CHECK(data);
    if (!data)
        return 1;
    // Country 2 ("Hole country") is around 0 N, 20-32 E.
    const auto camera = *make_camera({{0, 26}, 0, 90, 160, 120});
    auto fb = Framebuffer::create(160, 120);
    CHECK(fb);
    if (!fb)
        return 1;
    fb->clear(0x1234);
    const std::vector<Rgb565> before(fb->data(), fb->data() + 160 * 120);
    const auto drawn = draw_country_outline(*fb, camera, *data, 2, kCountryHighlight);
    CHECK(drawn >= 8); // an outer ring and a hole
    std::size_t changed = 0;
    for (int i = 0; i < 160 * 120; ++i) {
        if (fb->data()[i] != before[static_cast<std::size_t>(i)]) {
            ++changed;
            CHECK(fb->data()[i] == kCountryHighlight);
        }
    }
    CHECK(changed > 100);
    // The globe's far side is not drawn, and an unknown country draws nothing.
    fb->clear(0x1234);
    const auto behind = *make_camera({{0, 206}, 0, 90, 160, 120});
    CHECK(draw_country_outline(*fb, behind, *data, 2, kCountryHighlight) == 0);
    CHECK(draw_country_outline(*fb, camera, *data, 99, kCountryHighlight) == 0);
    for (int i = 0; i < 160 * 120; ++i)
        CHECK(fb->data()[i] == 0x1234);
    // Dither fill: only the diagonal pattern pixels change, inside the country, never in its hole.
    {
        fb->clear(0x1234);
        const auto painted = fill_country_dither(*fb, camera, *data, 2, kCountryHighlight);
        CHECK(painted > 200);
        std::size_t changed = 0;
        for (int y = 0; y < 120; ++y)
            for (int x = 0; x < 160; ++x) {
                if (fb->pixel(x, y) != 0x1234) {
                    ++changed;
                    CHECK(fb->pixel(x, y) == kCountryHighlight);
                    CHECK(((x + 2 * y) & 3) == 0);
                }
            }
        CHECK(changed == painted);
        // The country's hole (around 31 E on the equator, see test_countries) stays untouched.
        const auto hole = to_screen(camera, to_camera_space(camera, to_unit_vector({0, 31})));
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx)
                CHECK(fb->pixel(static_cast<int>(hole.x) + dx, static_cast<int>(hole.y) + dy) ==
                      0x1234);
        // Behind the globe, or an unknown id: nothing.
        fb->clear(0x1234);
        CHECK(fill_country_dither(*fb, behind, *data, 2, kCountryHighlight) == 0);
        CHECK(fill_country_dither(*fb, camera, *data, 99, kCountryHighlight) == 0);
        // Cut by the limb: the camera looks 85 degrees east of the country; it is still filled,
        // only inside the disc, and the fill never leaves the globe.
        const auto limb = *make_camera({{0, 111}, 0, 60, 160, 120});
        fb->clear(0x1234);
        const auto edge = fill_country_dither(*fb, limb, *data, 2, kCountryHighlight);
        CHECK(edge > 0);
        for (int y = 0; y < 120; ++y)
            for (int x = 0; x < 160; ++x)
                if (fb->pixel(x, y) != 0x1234) {
                    const double dx = x + 0.5 - limb.center_x, dy = y + 0.5 - limb.center_y;
                    CHECK(dx * dx + dy * dy <= limb.radius_px * limb.radius_px + 1.0);
                }
        // Independent oracle: every pattern pixel inside the disc is filled exactly when the
        // inverse-projected point is in the country (picked by the country scene). Pixels on the
        // boundary may differ; the clip along the limb must not.
        auto picker = CountryScene::create(*data);
        CHECK(picker);
        if (picker) {
            int total_expected = 0;
            auto big = Framebuffer::create(400, 300);
            CHECK(big);
            if (!big)
                return 1;
            for (const auto& cam : {*make_camera({{0, 100}, 0, 140, 400, 300}),
                                    *make_camera({{0, 106}, 0, 140, 400, 300}),
                                    *make_camera({{0, 112}, 0, 140, 400, 300}),
                                    *make_camera({{12, 116}, 0, 140, 400, 300})}) {
                big->clear(0x1234);
                fill_country_dither(*big, cam, *data, 2, kCountryHighlight);
                const auto in_country = [&](int x, int y) {
                    const double dx = (x + 0.5 - cam.center_x) / cam.radius_px;
                    const double dy = (cam.center_y - (y + 0.5)) / cam.radius_px;
                    if (dx * dx + dy * dy >= 0.98) // within about a pixel of the limb
                        return false;
                    const Vec3 c{dx, dy, std::sqrt(1.0 - dx * dx - dy * dy)};
                    const Vec3 w = cam.right * c.x + cam.up * c.y + cam.forward * c.z;
                    return picker->pick(from_unit_vector(w)) == 2u;
                };
                // A pattern pixel must be filled exactly when it is in the country; pixels next to
                // the boundary (where the oracle and the pixel grid can disagree) are skipped.
                int expected = 0, mismatches = 0;
                for (int y = 1; y < 299; ++y)
                    for (int x = 1; x < 399; ++x) {
                        if (((x + 2 * y) & 3) != 0)
                            continue;
                        // The oracle cannot resolve the outermost few pixels at the limb.
                        const double bx = (x + 0.5 - cam.center_x) / cam.radius_px;
                        const double by = (cam.center_y - (y + 0.5)) / cam.radius_px;
                        if (bx * bx + by * by >= 0.94)
                            continue;
                        const bool inside = in_country(x, y);
                        expected += inside;
                        const bool filled = big->pixel(x, y) == kCountryHighlight;
                        if (inside == filled)
                            continue;
                        const bool boundary =
                            in_country(x - 1, y) != inside || in_country(x + 1, y) != inside ||
                            in_country(x, y - 1) != inside || in_country(x, y + 1) != inside;
                        mismatches += !boundary;
                    }
                total_expected += expected;
                CHECK(mismatches <= 2);
            }
            CHECK(total_expected > 100);
        }
    }
    // Real data: Russia cut deeply by the limb. A sparse grid of pattern pixels must be filled
    // exactly when the inverse-projected point is in Russia (boundary pixels excepted). This is
    // what catches a missing or wrong limb arc, which the tiny test country is too small to show.
    {
        const auto real = read(root + "/data/countries.gpol");
        auto world = Countries::parse(real.data(), real.size());
        CHECK(world);
        if (!world)
            return 1;
        auto picker = CountryScene::create(*world);
        CHECK(picker);
        std::uint32_t russia = world->count();
        for (std::uint32_t i = 0; i < world->count(); ++i)
            if (world->name(i).find("Russia") != std::string_view::npos)
                russia = i;
        CHECK(russia < world->count());
        auto big = Framebuffer::create(400, 300);
        CHECK(big);
        if (!picker || !big || russia >= world->count())
            return 1;
        int total_samples = 0;
        // Views where the limb cuts Russia across the middle (the hidden part is large).
        for (const auto& cam : {*make_camera({{-20, 100}, 0, 140, 400, 300}),
                                *make_camera({{-20, 60}, 0, 140, 400, 300}),
                                *make_camera({{0, 100}, 0, 140, 400, 300})}) {
            big->clear(0x1234);
            CHECK(fill_country_dither(*big, cam, *world, russia, kCountryHighlight) > 100);
            const auto in_russia = [&](int x, int y) {
                const double dx = (x + 0.5 - cam.center_x) / cam.radius_px;
                const double dy = (cam.center_y - (y + 0.5)) / cam.radius_px;
                if (dx * dx + dy * dy >= 0.94)
                    return false;
                const Vec3 c{dx, dy, std::sqrt(1.0 - dx * dx - dy * dy)};
                const Vec3 w = cam.right * c.x + cam.up * c.y + cam.forward * c.z;
                return picker->pick(from_unit_vector(w)) == russia;
            };
            int inside_samples = 0, mismatches = 0;
            for (int y = 8; y < 292; y += 8)
                for (int x = 8; x < 392; x += 8) { // x, y multiples of 8 are pattern pixels
                    const double bx = (x + 0.5 - cam.center_x) / cam.radius_px;
                    const double by = (cam.center_y - (y + 0.5)) / cam.radius_px;
                    if (bx * bx + by * by >= 0.94)
                        continue;
                    const bool inside = in_russia(x, y);
                    inside_samples += inside;
                    if (inside != (big->pixel(x, y) == kCountryHighlight) &&
                        in_russia(x - 3, y) == inside && in_russia(x + 3, y) == inside &&
                        in_russia(x, y - 3) == inside && in_russia(x, y + 3) == inside)
                        ++mismatches;
                }
            total_samples += inside_samples;
            CHECK(mismatches <= 1);
        }
        CHECK(total_samples > 30);
    }
    return TEST_MAIN_RESULT();
}
