// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/bytes.hpp"

using namespace globe;

namespace {
constexpr std::uint8_t kConstBytes[4] = {0, 0, 0, 1};
static_assert(load_be32(kConstBytes) == 1, "usable at compile time");
static_assert(load_le32(kConstBytes) == 0x01000000U, "usable at compile time");
} // namespace

int main() {
    const std::uint8_t bytes[4] = {0x12, 0x34, 0x56, 0x78};
    CHECK(load_le16(bytes) == 0x3412);
    CHECK(load_be16(bytes) == 0x1234);
    CHECK(load_le32(bytes) == 0x78563412U);
    CHECK(load_be32(bytes) == 0x12345678U);

    std::uint8_t out[4] = {};
    store_le32(out, 0xA1B2C3D4U);
    CHECK(out[0] == 0xD4 && out[1] == 0xC3 && out[2] == 0xB2 && out[3] == 0xA1);
    store_be32(out, 0xA1B2C3D4U);
    CHECK(out[0] == 0xA1 && out[1] == 0xB2 && out[2] == 0xC3 && out[3] == 0xD4);
    store_le16(out, 0xBEEF);
    CHECK(out[0] == 0xEF && out[1] == 0xBE);
    store_be16(out, 0xBEEF);
    CHECK(out[0] == 0xBE && out[1] == 0xEF);

    // Round trips at the extremes, including values with the high bit set.
    const std::uint32_t values[] = {0U, 1U, 0x80000000U, 0xFFFFFFFFU, 0x01020304U};
    for (const std::uint32_t v : values) {
        store_le32(out, v);
        CHECK(load_le32(out) == v);
        store_be32(out, v);
        CHECK(load_be32(out) == v);
    }

    return TEST_MAIN_RESULT();
}
