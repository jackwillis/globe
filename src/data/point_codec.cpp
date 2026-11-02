// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/point_codec.hpp"

namespace globe {

namespace {

// Reads one zigzag varint at in[pos]; returns the signed value in `value`.
PointCodecError read_zigzag(const std::uint8_t* in, std::size_t size, std::size_t& pos,
                            std::int32_t& value) {
    std::uint32_t raw = 0;
    for (unsigned i = 0; i < 3; ++i) {
        if (pos >= size) {
            return PointCodecError::Truncated;
        }
        const std::uint8_t byte = in[pos++];
        raw |= static_cast<std::uint32_t>(byte & 0x7FU) << (7 * i);
        if ((byte & 0x80U) == 0) {
            if (i > 0 && byte == 0) {
                return PointCodecError::BadVarint; // trailing zero group: not the shortest form
            }
            value = static_cast<std::int32_t>(raw >> 1) ^ -static_cast<std::int32_t>(raw & 1U);
            return PointCodecError::Ok;
        }
    }
    return PointCodecError::BadVarint; // a fourth byte would be needed
}

} // namespace

PointCodecError decode_delta_points(const std::uint8_t* in, std::size_t size, std::size_t* consumed,
                                    std::uint16_t* out, std::size_t count) {
    std::size_t pos = 0;
    std::int32_t x = 0;
    std::int32_t y = 0;
    for (std::size_t i = 0; i < count; ++i) {
        std::int32_t dx = 0;
        std::int32_t dy = 0;
        PointCodecError e = read_zigzag(in, size, pos, dx);
        if (e != PointCodecError::Ok) {
            return e;
        }
        e = read_zigzag(in, size, pos, dy);
        if (e != PointCodecError::Ok) {
            return e;
        }
        x += dx;
        y += dy;
        if (x < 0 || x > 65535 || y < 0 || y > 65535) {
            return PointCodecError::OutOfRange;
        }
        out[2 * i] = static_cast<std::uint16_t>(x);
        out[2 * i + 1] = static_cast<std::uint16_t>(y);
    }
    if (consumed != nullptr) {
        *consumed = pos;
    }
    return PointCodecError::Ok;
}

} // namespace globe
