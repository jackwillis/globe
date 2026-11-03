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
    return TEST_MAIN_RESULT();
}
