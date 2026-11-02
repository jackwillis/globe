// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/bytes.hpp"
#include "globe/crc32.hpp"
#include "globe/places.hpp"
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
using namespace globe;
int main(int argc, char** argv) {
    const std::string root = argc > 1 ? argv[1] : ".";
    std::ifstream file(root + "/tests/data/places.gplc", std::ios::binary);
    std::vector<std::uint8_t> b((std::istreambuf_iterator<char>(file)), {});
    const auto data = Places::parse(b.data(), b.size());
    CHECK(data);
    if (!data)
        return 1;
    auto scene = PlaceScene::create(*data);
    CHECK(scene);
    if (!scene)
        return 1;
    const auto camera = make_camera({{0, 0}, 0, 50, 160, 120});
    CHECK(camera);
    if (!camera)
        return 1;
    CHECK(scene->pick(*camera, {80, 60}, 6) == 0);
    CHECK(!scene->pick(*camera, {90, 60}, 6));
    CHECK(!scene->pick(*camera, {105, 60}, 6));
    auto opposite = make_camera({{0, 180}, 0, 50, 160, 120});
    CHECK(opposite);
    if (opposite)
        CHECK(scene->pick(*opposite, {55, 60}, 6) == 1);
    CHECK(!scene->pick(*camera, {80, 60}, -1));
    auto fb = Framebuffer::create(160, 120);
    CHECK(fb);
    if (fb) {
        fb->clear(0);
        scene->draw(*fb, *camera);
        CHECK(fb->pixel(80, 60) != 0);
        CHECK(fb->pixel(0, 0) == 0);
        CHECK(fb->pixel(105, 60) == 0);
    }
    CHECK(place_card(*data, 0).title == "Front");
    CHECK(place_card(*data, 100).title.empty());
    for (std::size_t n = 0; n < b.size(); ++n)
        CHECK(!Places::parse(b.data(), n));
    for (std::size_t i = 0; i < b.size(); ++i) {
        auto x = b;
        x[i] ^= 1;
        CHECK(!Places::parse(x.data(), x.size()));
    }
    auto reject = [&](std::size_t at, std::uint32_t value) {
        auto x = b;
        store_le32(x.data() + at, value);
        store_le32(x.data() + 20, crc32_extend(crc32(x.data(), 20), x.data() + 24, x.size() - 24));
        CHECK(!Places::parse(x.data(), x.size()));
    };
    reject(8, 4097);
    reject(24, 0);
    reject(28, 180000001);
    reject(36, 1);
    reject(40, 96);
    reject(44, 1);
    reject(48, 1);
    return TEST_MAIN_RESULT();
}
