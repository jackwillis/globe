// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Usage: test_atlas [REPO_ROOT]   (default ".") - reads data/atlas.gatl and tests/data/tiny.gatl.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <new>
#include <string>
#include <utility>
#include <vector>

#include "atlas_util.hpp"
#include "check.hpp"
#include "globe/atlas.hpp"
#include "globe/bytes.hpp"
#include "globe/crc32.hpp"

using namespace globe;
using namespace atlas_util;

// Count heap allocations so the "parsing allocates nothing" claim is tested, not just stated.
namespace {
std::size_t g_allocations = 0;
}
void* operator new(std::size_t n) {
    ++g_allocations;
    void* p = std::malloc(n != 0 ? n : 1);
    if (p == nullptr) {
        std::abort();
    }
    return p;
}
void operator delete(void* p) noexcept {
    std::free(p);
}
void operator delete(void* p, std::size_t) noexcept {
    std::free(p);
}

namespace {

AtlasError error_of(const Bytes& b) {
    AtlasError e = AtlasError::None;
    const auto a = Atlas::parse(b.data(), b.size(), &e);
    CHECK(!a.has_value());
    return e;
}

bool read_file(const std::string& path, Bytes& out) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (f == nullptr) {
        return false;
    }
    std::uint8_t buf[4096];
    std::size_t got = 0;
    out.clear();
    while ((got = std::fread(buf, 1, sizeof buf, f)) > 0) {
        out.insert(out.end(), buf, buf + got);
    }
    std::fclose(f);
    return true;
}

// Everything the format promises about an accepted atlas, checked independently of parse().
bool invariants_hold(const Atlas& a) {
    AtlasCursor cur;
    AtlasPolyline p{};
    std::uint64_t total = 0;
    std::uint32_t count = 0;
    while (a.next(cur, p)) {
        ++count;
        if (p.layer != AtlasLayer::Coastline && p.layer != AtlasLayer::Border) {
            return false;
        }
        if (p.point_count < 2) {
            return false;
        }
        for (std::uint16_t i = 0; i < p.point_count; ++i) {
            const LatLon v = p.point(i);
            if (!(v.lat_deg >= -90.0 && v.lat_deg <= 90.0 && v.lon_deg >= -180.0 &&
                  v.lon_deg < 180.0)) {
                return false;
            }
        }
        total += p.point_count;
    }
    return count == a.polyline_count() && total == a.point_count();
}

const std::vector<Poly> kTinyPolys = {
    {1, {{0, 0}, {65535, 32768}, {32768, 16384}}},
    {2, {{1, 2}, {3, 4}}},
};

void test_crc32() {
    const std::uint8_t digits[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    CHECK(crc32(digits, 9) == 0xCBF43926U); // the standard CRC-32 check value
    CHECK(crc32(digits, 0) == 0U);
    const std::uint8_t one[] = {'a'};
    CHECK(crc32(one, 1) == 0xE8B7BE43U);
}

void test_tiny_fixture(const std::string& root) {
    Bytes file;
    CHECK(read_file(root + "/tests/data/tiny.gatl", file));
    // The Python builder's output and this file's own encoder agree byte for byte.
    CHECK(file == make_atlas(kTinyPolys, "tiny test atlas"));

    AtlasError e = AtlasError::BadMagic;
    const auto a = Atlas::parse(file.data(), file.size(), &e);
    CHECK(a.has_value() && e == AtlasError::None);
    if (!a) {
        return;
    }
    CHECK(a->polyline_count() == 2 && a->point_count() == 5);
    CHECK(a->provenance() == "tiny test atlas");
    AtlasCursor cur;
    AtlasPolyline p{};
    CHECK(a->next(cur, p));
    CHECK(p.layer == AtlasLayer::Coastline && p.point_count == 3);
    CHECK(p.point(0).lon_deg == -180.0 && p.point(0).lat_deg == -90.0);
    CHECK(p.point(1).lat_deg == 90.0 && p.point(1).lon_deg == 180.0 - 360.0 / 65536.0);
    CHECK(p.point(2).lon_deg == 0.0 && p.point(2).lat_deg == 0.0);
    CHECK(p.point(3).lon_deg == 0.0 && p.point(3).lat_deg == 0.0); // out of range -> (0, 0)
    CHECK(a->next(cur, p));
    CHECK(p.layer == AtlasLayer::Border && p.point_count == 2);
    CHECK(p.point(1).lon_deg == 3 * (360.0 / 65536.0) - 180.0);
    CHECK(!a->next(cur, p));
    CHECK(!a->next(cur, p)); // stays finished
    CHECK(invariants_hold(*a));
}

void test_real_atlas(const std::string& root) {
    Bytes file;
    CHECK(read_file(root + "/data/atlas.gatl", file));
    const std::size_t before = g_allocations;
    AtlasError e = AtlasError::BadMagic;
    const auto a = Atlas::parse(file.data(), file.size(), &e);
    CHECK(a.has_value());
    if (!a) {
        std::printf("parse failed: %s\n", atlas_error_name(e));
        return;
    }
    // Expected values come from the independent Python decoder (tests/test_atlas_builder.py).
    CHECK(a->polyline_count() == 467 && a->point_count() == 8120);
    std::uint64_t sum_lon_q = 0;
    std::uint64_t sum_lat_q = 0;
    int coast = 0;
    int borders = 0;
    AtlasCursor cur;
    AtlasPolyline p{};
    bool first = true;
    while (a->next(cur, p)) {
        (p.layer == AtlasLayer::Coastline ? coast : borders) += 1;
        for (std::uint16_t i = 0; i < p.point_count; ++i) {
            sum_lon_q += load_le16(p.points + 4 * i);
            sum_lat_q += load_le16(p.points + 4 * i + 2);
        }
        if (first) {
            CHECK(p.point_count == 11);
            CHECK(load_le16(p.points) == 2965 && load_le16(p.points + 2) == 2076);
            first = false;
        }
    }
    CHECK(coast == 134 && borders == 333);
    CHECK(sum_lon_q == 280833220U && sum_lat_q == 159366554U);
    // Last polyline's last point.
    AtlasCursor c2;
    AtlasPolyline last{};
    while (a->next(c2, p)) {
        last = p;
    }
    CHECK(last.layer == AtlasLayer::Border && last.point_count == 4);
    CHECK(load_le16(last.points + 4 * 3) == 46447 && load_le16(last.points + 4 * 3 + 2) == 22684);
    CHECK(a->provenance().find("public domain") != std::string_view::npos);
    CHECK(a->provenance().find("664449b39070027e") != std::string_view::npos);
    CHECK(invariants_hold(*a));
    // Parsing and walking the whole atlas allocated nothing.
    CHECK(g_allocations == before);
}

void test_header_errors() {
    const Bytes good = make_atlas(kTinyPolys, "t");
    CHECK(Atlas::parse(good.data(), good.size()).has_value());

    CHECK(Atlas::parse(nullptr, 100) == std::nullopt);
    CHECK(Atlas::parse(good.data(), 0) == std::nullopt);
    AtlasError e = AtlasError::None;
    CHECK(!Atlas::parse(good.data(), 23, &e) && e == AtlasError::TooShort);
    CHECK(!Atlas::parse(nullptr, 0, &e) && e == AtlasError::TooShort);

    Bytes b = good;
    b[0] = 'X';
    CHECK(error_of(b) == AtlasError::BadMagic);
    b = good;
    store_le16(b.data() + 4, 2);
    CHECK(error_of(b) == AtlasError::UnsupportedVersion);
    store_le16(b.data() + 4, 0);
    CHECK(error_of(b) == AtlasError::UnsupportedVersion);
    b = good;
    store_le16(b.data() + 6, 1);
    CHECK(error_of(b) == AtlasError::BadReserved);

    // Limits are checked before anything is sized or read.
    b = good;
    store_le32(b.data() + 8, kAtlasMaxPolylines + 1);
    CHECK(error_of(b) == AtlasError::TooLarge);
    store_le32(b.data() + 8, 0xFFFFFFFFU);
    CHECK(error_of(b) == AtlasError::TooLarge);
    b = good;
    store_le32(b.data() + 12, kAtlasMaxPoints + 1);
    CHECK(error_of(b) == AtlasError::TooLarge);
    store_le32(b.data() + 12, 0xFFFFFFFFU);
    CHECK(error_of(b) == AtlasError::TooLarge);
    b = good;
    store_le32(b.data() + 16, kAtlasMaxTextBytes + 1);
    CHECK(error_of(b) == AtlasError::TooLarge);
    store_le32(b.data() + 16, 0xFFFFFFFFU);
    CHECK(error_of(b) == AtlasError::TooLarge);

    // Counts at the limit but a file that is far too small: a size mismatch, not a crash.
    b = good;
    store_le32(b.data() + 8, kAtlasMaxPolylines);
    store_le32(b.data() + 12, kAtlasMaxPoints);
    store_le32(b.data() + 16, kAtlasMaxTextBytes);
    CHECK(error_of(b) == AtlasError::SizeMismatch);

    // Size must match exactly: short, long.
    b = good;
    b.pop_back();
    CHECK(error_of(b) == AtlasError::SizeMismatch);
    b = good;
    b.push_back(0);
    CHECK(error_of(b) == AtlasError::SizeMismatch);

    b = good;
    b[b.size() - 1] ^= 0x01; // body damage with an untouched CRC field
    CHECK(error_of(b) == AtlasError::BadChecksum);
    b = good;
    b[20] ^= 0x01; // CRC field damage
    CHECK(error_of(b) == AtlasError::BadChecksum);
}

void test_structural_errors() {
    const auto with = [](std::vector<Poly> polys, const std::string& text = "t") {
        return make_atlas(polys, text);
    };
    CHECK(error_of(with({{0, {{0, 0}, {1, 1}}}})) == AtlasError::BadLayer);
    CHECK(error_of(with({{3, {{0, 0}, {1, 1}}}})) == AtlasError::BadLayer);
    CHECK(error_of(with({{255, {{0, 0}, {1, 1}}}})) == AtlasError::BadLayer);
    CHECK(error_of(with({{1, {{0, 0}}}})) == AtlasError::BadPointCount);
    CHECK(error_of(with({{1, {}}})) == AtlasError::BadPointCount);
    CHECK(error_of(with({{1, {{0, 32769}, {1, 1}}}})) == AtlasError::BadLatitude);
    CHECK(error_of(with({{1, {{0, 65535}, {1, 1}}}})) == AtlasError::BadLatitude);
    CHECK(Atlas::parse(with({{1, {{0, 32768}, {65535, 0}}}}).data(), 24 + 4 + 8 + 1).has_value());

    // Reserved table byte.
    Bytes b = with({{1, {{0, 0}, {1, 1}}}});
    b[25] = 1;
    refresh_crc(b);
    CHECK(error_of(b) == AtlasError::BadPolyline);

    // Table counts disagreeing with the header's total, both directions.
    b = with(kTinyPolys);
    store_le32(b.data() + 12, 6);
    refresh_crc(b);
    CHECK(error_of(b) == AtlasError::SizeMismatch); // the size no longer fits either
    b = with(kTinyPolys);
    store_le16(b.data() + 24 + 2, 2); // first polyline claims 2 points instead of 3
    refresh_crc(b);
    CHECK(error_of(b) == AtlasError::BadPointCount);
    b = with(kTinyPolys);
    store_le16(b.data() + 24 + 2, 4); // claims 4: running total passes the header's 5
    store_le16(b.data() + 24 + 4 + 2, 3);
    refresh_crc(b);
    CHECK(error_of(b) == AtlasError::BadPointCount);

    // Empty atlas is valid (zero polylines, zero points).
    const Bytes empty = make_atlas({}, "");
    const auto a = Atlas::parse(empty.data(), empty.size());
    CHECK(a.has_value() && a->polyline_count() == 0 && a->point_count() == 0);
    AtlasCursor cur;
    AtlasPolyline p{};
    CHECK(a && !a->next(cur, p));
}

void test_text_validation() {
    const std::vector<Poly> one = {{1, {{0, 0}, {1, 1}}}};
    const auto text_error = [&](const std::string& t) {
        const Bytes b = make_atlas(one, t);
        AtlasError e = AtlasError::None;
        const bool ok = Atlas::parse(b.data(), b.size(), &e).has_value();
        return ok ? AtlasError::None : e;
    };
    CHECK(text_error("plain ascii") == AtlasError::None);
    CHECK(text_error("") == AtlasError::None);
    CHECK(text_error("euro \xE2\x82\xAC sign") == AtlasError::None);
    CHECK(text_error("\xF0\x9F\x98\x80") == AtlasError::None);
    CHECK(text_error("\xF4\x8F\xBF\xBF") == AtlasError::None); // U+10FFFF
    CHECK(text_error(std::string("a\0b", 3)) == AtlasError::BadText);
    CHECK(text_error("\x80") == AtlasError::BadText);             // lone continuation
    CHECK(text_error("\xC0\x80") == AtlasError::BadText);         // overlong NUL
    CHECK(text_error("\xC1\xBF") == AtlasError::BadText);         // overlong
    CHECK(text_error("\xE0\x80\xAF") == AtlasError::BadText);     // overlong
    CHECK(text_error("\xED\xA0\x80") == AtlasError::BadText);     // surrogate
    CHECK(text_error("\xF4\x90\x80\x80") == AtlasError::BadText); // above U+10FFFF
    CHECK(text_error("\xF5\x80\x80\x80") == AtlasError::BadText);
    CHECK(text_error("\xFF") == AtlasError::BadText);
    CHECK(text_error("\xE2\x82") == AtlasError::BadText);     // truncated at the end
    CHECK(text_error("\xE2\x82\x41") == AtlasError::BadText); // bad continuation
}

// Any single changed byte must be rejected: the CRC covers the body, and every header field is
// either validated or part of the exact-size equation.
void test_every_single_byte_change_is_rejected(const Bytes& good, bool exhaustive_values) {
    for (std::size_t i = 0; i < good.size(); ++i) {
        for (int v = 0; v < 256; ++v) {
            if (v == good[i]) {
                continue;
            }
            if (!exhaustive_values && (v & (v - 1)) != 0) {
                continue; // only single-bit patterns (the v values that are powers of two)
            }
            Bytes b = good;
            b[i] = static_cast<std::uint8_t>(v);
            if (Atlas::parse(b.data(), b.size()).has_value()) {
                std::printf("accepted: byte %zu changed to %d\n", i, v);
                CHECK(false);
                return;
            }
        }
    }
}

void test_every_truncation_and_extension_is_rejected(const Bytes& good, std::size_t limit) {
    for (std::size_t n = 0; n < good.size() && n < limit; ++n) {
        CHECK(!Atlas::parse(good.data(), n).has_value());
    }
    for (std::size_t n = good.size() > limit ? good.size() - limit : 0; n < good.size(); ++n) {
        CHECK(!Atlas::parse(good.data(), n).has_value());
    }
    Bytes longer = good;
    longer.push_back(0);
    CHECK(!Atlas::parse(longer.data(), longer.size()).has_value());
}

// With the CRC recomputed after each change, the structural checks must carry the load: the
// result is either a rejection or an atlas that satisfies every documented invariant, and each
// kind of structural error is actually reached.
void test_structural_mutation_with_valid_crc() {
    const Bytes good = make_atlas(kTinyPolys, "tiny test atlas");
    std::map<AtlasError, int> seen;
    int accepted = 0;
    for (std::size_t i = 24; i < good.size(); ++i) {
        for (int v = 0; v < 256; ++v) {
            if (v == good[i]) {
                continue;
            }
            Bytes b = good;
            b[i] = static_cast<std::uint8_t>(v);
            refresh_crc(b);
            AtlasError e = AtlasError::None;
            const auto a = Atlas::parse(b.data(), b.size(), &e);
            if (a) {
                ++accepted;
                CHECK(invariants_hold(*a));
            } else {
                ++seen[e];
            }
        }
    }
    CHECK(accepted > 0); // e.g. a different but valid coordinate
    CHECK(seen[AtlasError::BadLayer] > 0);
    CHECK(seen[AtlasError::BadPolyline] > 0);
    CHECK(seen[AtlasError::BadPointCount] > 0);
    CHECK(seen[AtlasError::BadLatitude] > 0);
    CHECK(seen[AtlasError::BadText] > 0);
}

} // namespace

int main(int argc, char** argv) {
    const std::string root = argc > 1 ? argv[1] : ".";
    test_crc32();
    test_tiny_fixture(root);
    test_real_atlas(root);
    test_header_errors();
    test_structural_errors();
    test_text_validation();
    test_structural_mutation_with_valid_crc();

    const Bytes tiny = make_atlas(kTinyPolys, "tiny test atlas");
    test_every_single_byte_change_is_rejected(tiny, true);
    test_every_truncation_and_extension_is_rejected(tiny, tiny.size());

    Bytes real;
    if (read_file(root + "/data/atlas.gatl", real)) {
        // The real file is large: bit flips at every header and table-start byte, a stride
        // through the body, and the last bytes; truncations near both ends.
        test_every_truncation_and_extension_is_rejected(real, 3000);
        for (std::size_t i = 0; i < real.size();
             i += (i < 200 || i + 200 > real.size()) ? 1 : 211) {
            for (int bit = 0; bit < 8; ++bit) {
                Bytes b = real;
                b[i] ^= static_cast<std::uint8_t>(1U << bit);
                CHECK(!Atlas::parse(b.data(), b.size()).has_value());
            }
        }
    } else {
        CHECK(false);
    }
    return TEST_MAIN_RESULT();
}
