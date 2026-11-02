// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/countries.hpp"
#include "globe/bytes.hpp"
#include "globe/crc32.hpp"
#include <cstring>
namespace globe {
namespace {
std::int32_t signed32(const std::uint8_t* p) {
    const auto n = load_le32(p);
    return n <= 0x7fffffffU ? static_cast<std::int32_t>(n) : -1 - static_cast<std::int32_t>(~n);
}
} // namespace
std::optional<Countries> Countries::parse(const std::uint8_t* b, std::size_t size) {
    if (!b || size < 32 || std::memcmp(b, "GPOL", 4) != 0 || load_le16(b + 4) != 1 ||
        load_le16(b + 6) != 0 || load_le32(b + 24) != 0)
        return std::nullopt;
    const auto n = load_le32(b + 8), r = load_le32(b + 12), p = load_le32(b + 16),
               t = load_le32(b + 20);
    if (!n || n > 4096 || !r || r > 65536 || !p || p > 1000000 || !t || t > 1048576)
        return std::nullopt;
    Countries out;
    out.data_ = b;
    out.count_ = n;
    out.rings_ = r;
    out.points_ = p;
    out.ring_offset_ = 32 + static_cast<std::size_t>(n) * 16;
    out.point_offset_ = out.ring_offset_ + static_cast<std::size_t>(r) * 16;
    out.text_offset_ = out.point_offset_ + static_cast<std::size_t>(p) * 8;
    if (size != out.text_offset_ + t ||
        load_le32(b + 28) != crc32_extend(crc32(b, 28), b + 32, size - 32))
        return std::nullopt;
    std::uint32_t next_ring = 0, next_text = 0, next_point = 0;
    for (std::uint32_t i = 0; i < n; ++i) {
        const auto* rec = b + 32 + static_cast<std::size_t>(i) * 16;
        auto c = out.country(i);
        auto text = load_le32(rec + 8), length = load_le32(rec + 12);
        if (c.first != next_ring || !c.count || c.count > r - next_ring || text != next_text ||
            !length || length > 95 || length > t - next_text)
            return std::nullopt;
        for (std::uint32_t j = 0; j < length; ++j)
            if (b[out.text_offset_ + text + j] < 32 || b[out.text_offset_ + text + j] > 126)
                return std::nullopt;
        next_text += length;
        for (std::uint32_t j = c.first; j < c.first + c.count; ++j) {
            const auto ring = out.ring(j);
            if (ring.first != next_point || ring.count < 3 || ring.count > p - next_point ||
                ring.hole > 1)
                return std::nullopt;
            if ((!ring.hole && ring.parent != j) ||
                (ring.hole &&
                 (ring.parent < c.first || ring.parent >= j || out.ring(ring.parent).hole)))
                return std::nullopt;
            next_point += ring.count;
        }
        next_ring += c.count;
    }
    if (next_ring != r || next_point != p || next_text != t)
        return std::nullopt;
    for (std::uint32_t i = 0; i < p; ++i) {
        const auto* q = b + out.point_offset_ + static_cast<std::size_t>(i) * 8;
        auto lon = signed32(q), lat = signed32(q + 4);
        if (lon < -180000000 || lon > 180000000 || lat < -90000000 || lat > 90000000)
            return std::nullopt;
    }
    return out;
}
CountryRecord Countries::country(std::uint32_t id) const {
    if (id >= count_)
        return {};
    const auto* b = data_ + 32 + static_cast<std::size_t>(id) * 16;
    return {load_le32(b), load_le32(b + 4)};
}
CountryRing Countries::ring(std::uint32_t id) const {
    if (id >= rings_)
        return {};
    const auto* b = data_ + ring_offset_ + static_cast<std::size_t>(id) * 16;
    return {load_le32(b), load_le32(b + 4), load_le32(b + 8), load_le32(b + 12)};
}
LatLon Countries::point(std::uint32_t id) const {
    if (id >= points_)
        return {};
    const auto* b = data_ + point_offset_ + static_cast<std::size_t>(id) * 8;
    return {signed32(b + 4) / 1e6, signed32(b) / 1e6};
}
std::string_view Countries::name(std::uint32_t id) const {
    if (id >= count_)
        return {};
    const auto* b = data_ + 32 + static_cast<std::size_t>(id) * 16;
    return {reinterpret_cast<const char*>(data_ + text_offset_ + load_le32(b + 8)),
            load_le32(b + 12)};
}
} // namespace globe
