// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/places.hpp"
#include "globe/bytes.hpp"
#include "globe/crc32.hpp"
#include <cstring>
namespace globe {
namespace {
std::int32_t signed32(const std::uint8_t* p) {
    const auto v = load_le32(p);
    return v <= 0x7fffffffU ? static_cast<std::int32_t>(v) : -1 - static_cast<std::int32_t>(~v);
}
} // namespace
std::optional<Places> Places::parse(const std::uint8_t* b, std::size_t size) {
    if (!b || size < 24 || std::memcmp(b, "GPLC", 4) != 0 || load_le16(b + 4) != 1 ||
        load_le16(b + 6) || load_le32(b + 16))
        return std::nullopt;
    const auto n = load_le32(b + 8), text = load_le32(b + 12);
    if (!n || n > 4096 || !text || text > 1048576)
        return std::nullopt;
    Places out;
    out.data_ = b;
    out.count_ = n;
    out.text_offset_ = 24 + static_cast<std::size_t>(n) * 24;
    if (size != out.text_offset_ + text ||
        load_le32(b + 20) != crc32_extend(crc32(b, 20), b + 24, size - 24))
        return std::nullopt;
    std::uint32_t prev = 0, next = 0;
    for (std::uint32_t i = 0; i < n; ++i) {
        const auto* r = b + 24 + static_cast<std::size_t>(i) * 24;
        const auto qid = load_le32(r), offset = load_le32(r + 12), length = load_le32(r + 16);
        const auto lon = signed32(r + 4), lat = signed32(r + 8);
        if (qid <= prev || lon < -180000000 || lon > 180000000 || lat < -90000000 ||
            lat > 90000000 || offset != next || !length || length > 95 || length > text - next ||
            load_le32(r + 20))
            return std::nullopt;
        for (std::uint32_t j = 0; j < length; ++j)
            if (b[out.text_offset_ + offset + j] < 32 || b[out.text_offset_ + offset + j] > 126)
                return std::nullopt;
        prev = qid;
        next += length;
    }
    if (next != text)
        return std::nullopt;
    return out;
}
std::uint32_t Places::qid(std::uint32_t id) const {
    return id < count_ ? load_le32(data_ + 24 + static_cast<std::size_t>(id) * 24) : 0;
}
LatLon Places::point(std::uint32_t id) const {
    if (id >= count_)
        return {};
    const auto* r = data_ + 24 + static_cast<std::size_t>(id) * 24;
    return {signed32(r + 8) / 1e6, signed32(r + 4) / 1e6};
}
std::string_view Places::name(std::uint32_t id) const {
    if (id >= count_)
        return {};
    const auto* r = data_ + 24 + static_cast<std::size_t>(id) * 24;
    return {reinterpret_cast<const char*>(data_ + text_offset_ + load_le32(r + 12)),
            load_le32(r + 16)};
}
} // namespace globe
