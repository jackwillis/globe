// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/bytes.hpp"
#include "globe/landclasses.hpp"
#include <cstdio>
#include <vector>
using namespace globe;
int main() {
    auto* file = std::fopen("data/landclasses-1deg.ggrd", "rb");
    CHECK(file);
    if (!file)
        return 1;
    std::vector<std::uint8_t> bytes;
    int c;
    while ((c = std::fgetc(file)) != EOF)
        bytes.push_back(static_cast<std::uint8_t>(c));
    std::fclose(file);
    auto grid = LandClasses::decode(bytes.data(), bytes.size());
    CHECK(grid);
    if (!grid)
        return 1;
    for (auto point : {std::pair{23.5, 13.5}, std::pair{23.5, 45.5}, std::pair{42.5, 104.5},
                       std::pair{-24.5, 134.5}})
        CHECK(grid->at(point.first, point.second) == LandClass::Desert);
    CHECK(grid->at(-3.5, -62.5) == LandClass::Forest);
    CHECK(grid->at(.5, 22.5) == LandClass::Forest);
    CHECK(grid->at(75.5, -42.5) == LandClass::Unknown); // No unreviewed Greenland ice semantics.
    CHECK(grid->at(100, 0) == LandClass::Unknown);
    for (std::size_t n = 0; n < bytes.size(); ++n)
        CHECK(!LandClasses::decode(bytes.data(), n));
    auto wrong_shape = bytes;
    store_le32(wrong_shape.data() + 8, 180);
    store_le32(wrong_shape.data() + 12, 360);
    CHECK(!LandClasses::decode(wrong_shape.data(), wrong_shape.size()));
    auto damaged = bytes;
    damaged.back() ^= 1;
    CHECK(!LandClasses::decode(damaged.data(), damaged.size()));
    auto* invalid = std::fopen("tests/data/landclass-invalid.ggrd", "rb");
    CHECK(invalid);
    if (invalid) {
        std::vector<std::uint8_t> out;
        int ch;
        while ((ch = std::fgetc(invalid)) != EOF)
            out.push_back(static_cast<std::uint8_t>(ch));
        std::fclose(invalid);
        // Valid stream and CRC, every decoded class is 5 (out of the defined 0..4 range).
        CHECK(!LandClasses::decode(out.data(), out.size()));
    }
    bytes.push_back(0);
    CHECK(!LandClasses::decode(bytes.data(), bytes.size()));
    return TEST_MAIN_RESULT();
}
