// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/bytes.hpp"
#include "globe/countries.hpp"
#include "globe/crc32.hpp"
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
using namespace globe;
std::vector<std::uint8_t> read(const std::string& path) {
    auto* f = std::fopen(path.c_str(), "rb");
    if (!f)
        return {};
    std::fseek(f, 0, SEEK_END);
    auto n = std::ftell(f);
    std::rewind(f);
    std::vector<std::uint8_t> b(static_cast<std::size_t>(n));
    const auto got = std::fread(b.data(), 1, b.size(), f);
    std::fclose(f);
    CHECK(got == b.size());
    return b;
}
int main(int argc, char** argv) {
    std::string root = argc > 1 ? argv[1] : ".";
    auto bytes = read(root + "/tests/data/countries.gpol");
    auto data = Countries::parse(bytes.data(), bytes.size());
    CHECK(data);
    if (!data)
        return 1;
    auto scene = CountryScene::create(*data);
    CHECK(scene);
    if (!scene)
        return 1;
    auto pick = [&](double lat, double lon) { return scene->pick({lat, lon}); };
    CHECK(pick(0, 175) == 0);
    CHECK(pick(0, -175) == 0);
    CHECK(!pick(0, 0));
    CHECK(pick(88, 0) == 1);
    CHECK(pick(88, 179) == 1);
    CHECK(!pick(-88, 0));
    CHECK(pick(0, 25) == 2);
    CHECK(!pick(0, 30));
    CHECK(!pick(0, 32));
    CHECK(pick(0, 20) == 2); // outer edge included; hole boundary excluded
    CHECK(!pick(NAN, 0));
    const auto card = country_card(*data, 2);
    CHECK(card.title == "Hole country" && card.line_count > 0);
    CHECK(country_card(*data, 99).title.empty());
    for (std::size_t n = 0; n < bytes.size(); ++n)
        CHECK(!Countries::parse(bytes.data(), n));
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        auto b = bytes;
        b[i] ^= 1;
        CHECK(!Countries::parse(b.data(), b.size()));
    }
    auto broken = bytes;
    store_le32(broken.data() + 8, 4097);
    store_le32(broken.data() + 28,
               crc32_extend(crc32(broken.data(), 28), broken.data() + 32, broken.size() - 32));
    CHECK(!Countries::parse(broken.data(), broken.size()));
    auto reject = [&](std::size_t at, std::uint32_t value) {
        auto b = bytes;
        store_le32(b.data() + at, value);
        store_le32(b.data() + 28, crc32_extend(crc32(b.data(), 28), b.data() + 32, b.size() - 32));
        CHECK(!Countries::parse(b.data(), b.size()));
    };
    reject(32, 1);  // country ring partition
    reject(44, 96); // name limit
    const std::size_t rings = 32 + 3 * 16;
    reject(rings, 1);                  // point partition
    reject(rings + 4, 2);              // short ring
    reject(rings + 8, 2);              // unknown hole flag
    reject(rings + 12, 1);             // exterior must parent itself
    reject(rings + 3 * 16 + 12, 3);    // hole cannot parent itself
    reject(rings + 4 * 16, 180000001); // out-of-range coordinate
    auto trailing = bytes;
    trailing.push_back(0);
    store_le32(trailing.data() + 28, crc32_extend(crc32(trailing.data(), 28), trailing.data() + 32,
                                                  trailing.size() - 32));
    CHECK(!Countries::parse(trailing.data(), trailing.size()));
    auto realbytes = read(root + "/data/countries.gpol");
    auto real = Countries::parse(realbytes.data(), realbytes.size());
    CHECK(real);
    if (real) {
        auto world = CountryScene::create(*real);
        CHECK(world);
        if (world)
            for (auto query : {LatLon{38, -97}, LatLon{48, 2}, LatLon{-89, 0}, LatLon{60, 100}}) {
                auto id = world->pick(query);
                CHECK(id);
                if (id)
                    std::printf("%s\n", std::string(real->name(*id)).c_str());
            }
    }
    return TEST_MAIN_RESULT();
}
