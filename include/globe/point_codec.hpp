// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Delta-varint decoder for polylines of 16-bit quantized points such as the atlas's (GL-29
// prototype, not used by any front end yet). Per polyline, a point is stored as two zigzag varints,
// the change in x then the change in y from the previous point, starting from (0, 0); so the first
// point is absolute. A varint is 7 bits per byte, low group first, high bit set on all but the last
// byte; the canonical (shortest) form is required and a value takes at most 3 bytes.
// The matching encoder is tools/grid-codec/grid_codec.py.
#pragma once

#include <cstddef>
#include <cstdint>

namespace globe {

enum class PointCodecError : std::uint8_t {
    Ok,
    Truncated, // input ended inside a point
    BadVarint, // longer than 3 bytes, or not the shortest encoding
    OutOfRange // a decoded coordinate left 0..65535
};

// Decodes `count` points into out[0 .. 2*count) as x0,y0,x1,y1,... from in[0 .. size). On success
// `*consumed` is the number of bytes used (the caller continues with the next polyline there).
// On error `*consumed` and the output are unspecified. `out` must hold 2*count values.
PointCodecError decode_delta_points(const std::uint8_t* in, std::size_t size, std::size_t* consumed,
                                    std::uint16_t* out, std::size_t count);

} // namespace globe
