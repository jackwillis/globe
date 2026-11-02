// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Test helper: builds GATL atlas bytes in memory (mirrors tools/atlas-builder/build_atlas.py).
#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "globe/bytes.hpp"
#include "globe/crc32.hpp"

namespace atlas_util {

using globe::crc32;
using globe::store_le16;
using globe::store_le32;

using Bytes = std::vector<std::uint8_t>;
using Point = std::pair<std::uint16_t, std::uint16_t>; // lon_q, lat_q

struct Poly {
    std::uint8_t layer;
    std::vector<Point> points;
};

inline void put16(Bytes& b, std::uint16_t v) {
    std::uint8_t t[2];
    store_le16(t, v);
    b.insert(b.end(), t, t + 2);
}
inline void put32(Bytes& b, std::uint32_t v) {
    std::uint8_t t[4];
    store_le32(t, v);
    b.insert(b.end(), t, t + 4);
}

// Fixes up the CRC field of a (possibly mutated) file so structural checks get exercised.
inline void refresh_crc(Bytes& b) {
    if (b.size() >= 24) {
        store_le32(b.data() + 20, crc32(b.data() + 24, b.size() - 24));
    }
}

inline Bytes make_atlas(const std::vector<Poly>& polys, const std::string& text) {
    Bytes body;
    std::uint32_t n = 0;
    for (const Poly& p : polys) {
        body.push_back(p.layer);
        body.push_back(0);
        put16(body, static_cast<std::uint16_t>(p.points.size()));
        n += static_cast<std::uint32_t>(p.points.size());
    }
    for (const Poly& p : polys) {
        for (const Point& pt : p.points) {
            put16(body, pt.first);
            put16(body, pt.second);
        }
    }
    body.insert(body.end(), text.begin(), text.end());
    Bytes out = {'G', 'A', 'T', 'L'};
    put16(out, 1);
    put16(out, 0);
    put32(out, static_cast<std::uint32_t>(polys.size()));
    put32(out, n);
    put32(out, static_cast<std::uint32_t>(text.size()));
    put32(out, crc32(body.data(), body.size()));
    out.insert(out.end(), body.begin(), body.end());
    return out;
}

} // namespace atlas_util
