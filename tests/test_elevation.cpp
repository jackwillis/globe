// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/bytes.hpp"
#include "globe/crc32.hpp"
#include "globe/elevation.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <vector>
namespace {
std::size_t allocations = 0;
}
[[gnu::noinline]] void* operator new(std::size_t n) {
    ++allocations;
    void* p = std::malloc(n ? n : 1);
    if (!p)
        std::abort();
    return p;
}
[[gnu::noinline]] void operator delete(void* p) noexcept {
    std::free(p);
}
[[gnu::noinline]] void operator delete(void* p, std::size_t) noexcept {
    std::free(p);
}
using namespace globe;
namespace {
using Bytes = std::vector<std::uint8_t>;
void checksum(Bytes& bytes) {
    const auto first = crc32(bytes.data(), 36);
    store_le32(bytes.data() + 36, crc32_extend(first, bytes.data() + 40, bytes.size() - 40));
}
Bytes fixture() {
    Bytes b(558, 0);
    std::memcpy(b.data(), "GELV", 4);
    store_le16(b.data() + 4, 1);
    store_le32(b.data() + 8, 3);
    store_le32(b.data() + 12, 2);
    store_le32(b.data() + 16, static_cast<std::uint32_t>(-180000000));
    store_le32(b.data() + 20, static_cast<std::uint32_t>(-90000000));
    store_le32(b.data() + 24, 180000000);
    store_le32(b.data() + 28, 90000000);
    store_le16(b.data() + 32, 256);
    for (unsigned i = 0; i < 256; ++i)
        store_le16(b.data() + 40 + 2 * i, static_cast<std::uint16_t>(static_cast<int>(i) - 128));
    for (unsigned i = 0; i < 6; ++i)
        b[552 + i] = static_cast<std::uint8_t>(i * 51);
    checksum(b);
    return b;
}
void test_format() {
    auto b = fixture();
    const auto before = allocations;
    const auto grid = Elevation::parse(b.data(), b.size());
    CHECK(allocations == before);
    CHECK(grid && grid->width() == 3 && grid->height() == 2);
    if (!grid)
        return;
    CHECK(grid->index(0, 0) == 0 && grid->meters(0, 0) == -128);
    CHECK(grid->index(2, 1) == 255 && grid->meters(2, 1) == 127);
    CHECK(grid->index(3, 0) == 0 && grid->meters(0, 2) == 0);
    CHECK(grid->west_deg() == -180 && grid->south_deg() == -90);
    CHECK(grid->east_deg() == 180 && grid->north_deg() == 90);
    CHECK(grid->cells() == b.data() + 552); // zero-copy
    for (std::size_t n = 0; n < b.size(); ++n)
        CHECK(!Elevation::parse(b.data(), n));
    b.push_back(0);
    CHECK(!Elevation::parse(b.data(), b.size()));
    checksum(b); // a valid CRC must not excuse trailing bytes
    CHECK(!Elevation::parse(b.data(), b.size()));
    b.pop_back();
    checksum(b);
    for (std::size_t i = 0; i < b.size(); ++i) {
        b[i] ^= 1;
        CHECK(!Elevation::parse(b.data(), b.size()));
        b[i] ^= 1;
    }
    CHECK(!Elevation::parse(nullptr, b.size()));
    for (const auto pair : {std::pair<unsigned, unsigned>{4, 0},
                            {6, 1},
                            {8, 0},
                            {12, 0},
                            {24, static_cast<unsigned>(-180000000)},
                            {28, static_cast<unsigned>(-90000000)},
                            {32, 0},
                            {34, 1}}) {
        auto bad = b;
        store_le32(bad.data() + pair.first, pair.second);
        checksum(bad);
        CHECK(!Elevation::parse(bad.data(), bad.size()));
    }
    auto unordered = b;
    store_le16(unordered.data() + 42, 32767);
    checksum(unordered);
    CHECK(!Elevation::parse(unordered.data(), unordered.size()));
    auto huge = b;
    store_le32(huge.data() + 8, 0xFFFFFFFFU);
    checksum(huge);
    CHECK(!Elevation::parse(huge.data(), huge.size()));
}
void test_real_pack(const std::string& root) {
    auto* file = std::fopen((root + "/data/elevation-1deg.gelv").c_str(), "rb");
    CHECK(file);
    if (!file)
        return;
    Bytes bytes;
    int c;
    while ((c = std::fgetc(file)) != EOF)
        bytes.push_back(static_cast<std::uint8_t>(c));
    std::fclose(file);
    const auto grid = Elevation::parse(bytes.data(), bytes.size());
    CHECK(grid && grid->width() == 360 && grid->height() == 180);
    if (grid) {
        CHECK(grid->meters(0, 0) > 0);   // Antarctic ice surface: row zero is south
        CHECK(grid->meters(0, 179) < 0); // Arctic ocean
        CHECK(grid->value(128) == 0);
    }
}
void test_python_fixture(const std::string& root) {
    std::FILE* file = std::fopen((root + "/tests/data/elevation.gelv").c_str(), "rb");
    CHECK(file != nullptr);
    if (!file)
        return;
    Bytes bytes;
    int c = 0;
    while ((c = std::fgetc(file)) != EOF)
        bytes.push_back(static_cast<std::uint8_t>(c));
    std::fclose(file);
    const auto grid = Elevation::parse(bytes.data(), bytes.size());
    CHECK(grid && grid->width() == 3 && grid->height() == 2);
    if (grid) {
        CHECK(grid->meters(0, 0) == -11000);
        CHECK(grid->meters(2, 1) == 9000);
    }
}
} // namespace
int main(int argc, char** argv) {
    test_real_pack(argc > 1 ? argv[1] : ".");
    test_format();
    test_python_fixture(argc > 1 ? argv[1] : ".");
    return TEST_MAIN_RESULT();
}
