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
    // Dot boxes cover the painted 5x5 dot plus a pixel, front side only.
    std::vector<PlacedLabel> dots;
    scene->dot_boxes(*camera, dots);
    CHECK(dots.size() == 1 && dots[0].candidate == 0 && dots[0].box == (LabelBox{77, 57, 84, 64}));
    if (opposite) {
        scene->dot_boxes(*opposite, dots);
        CHECK(dots.size() == 1 && dots[0].candidate == 1);
    }
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
    // v1 places carry no tier; v2 places report the stored one, and it must be 0..3.
    CHECK(data->tier(0) == kLastPlaceTier && data->tier(1) == kLastPlaceTier);
    std::ifstream file2(root + "/tests/data/places-v2.gplc", std::ios::binary);
    std::vector<std::uint8_t> v2((std::istreambuf_iterator<char>(file2)), {});
    const auto tiered = Places::parse(v2.data(), v2.size());
    CHECK(tiered);
    if (!tiered)
        return 1;
    CHECK(tiered->tier(0) == 0 && tiered->tier(1) == 3 && tiered->tier(2) == kLastPlaceTier);
    for (std::size_t i = 0; i < v2.size(); ++i) {
        auto x = v2;
        x[i] ^= 1;
        CHECK(!Places::parse(x.data(), x.size()));
    }
    auto reject_v2 = [&](std::size_t at, std::uint32_t value) {
        auto x = v2;
        store_le32(x.data() + at, value);
        store_le32(x.data() + 20, crc32_extend(crc32(x.data(), 20), x.data() + 24, x.size() - 24));
        CHECK(!Places::parse(x.data(), x.size()));
    };
    reject_v2(44, 4); // tier above 3
    reject_v2(68, 99);
    // Candidates follow pack order, beside the dot, ranked by tier.
    const auto labels = tiered->label_candidates();
    CHECK(labels.size() == 2 && labels[0].text == "Front" && labels[0].priority == 0 &&
          labels[1].priority == -3 && labels[0].side == LabelSide::right &&
          labels[0].role == LabelRole::point);
    // Zoom picks the tiers: a whole globe names only the largest cities.
    const auto whole = make_camera({{0, 0}, 0, 50, 160, 120});
    const auto near = make_camera({{0, 0}, 0, 600, 160, 120});
    CHECK(whole && near);
    if (whole && near) {
        CHECK(max_place_tier(*whole) == 0 && max_place_tier(*near) == kLastPlaceTier);
        std::vector<LabelCandidate> picked;
        select_place_labels(*tiered, labels, *whole, picked);
        CHECK(picked.size() == 1 && picked[0].text == "Front");
        select_place_labels(*tiered, labels, *near, picked);
        CHECK(picked.size() == 2);
        select_place_labels(*tiered, labels, *near, picked, 0, kLastLargeCityTier);
        CHECK(picked.size() == 1 && picked[0].text == "Front");
        select_place_labels(*tiered, labels, *near, picked, kLastLargeCityTier + 1);
        CHECK(picked.size() == 1 && picked[0].text == "Back");
    }
    const auto step = [](double radius) {
        return max_place_tier(*make_camera({{0, 0}, 0, radius, 100, 100}));
    };
    CHECK(step(79) == 0 && step(80) == 1 && step(139) == 1 && step(140) == 2 && step(259) == 2 &&
          step(260) == kLastPlaceTier);
    return TEST_MAIN_RESULT();
}
