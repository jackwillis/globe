// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Converts the RGB565 framebuffer into the pixel layout a window system asked for (bits per
// pixel, channel masks, byte order). No platform types: an adapter fills in PixelFormat from its
// visual (X11) or pixel map (QuickDraw GWorld), so one tested converter serves every front end
// and the conversion can be tested without a display.
//
// Channels are scaled to the mask width with rounding so that full intensity stays full
// (0x1F -> all ones) and zero stays zero. Padding bits are written as zero.
#pragma once

#include <cstddef>
#include <cstdint>

#include "globe/framebuffer.hpp"

namespace globe {

struct PixelFormat {
    int bits_per_pixel;     // 16, 24 or 32
    std::uint32_t red_mask; // each mask: one contiguous run of bits within bits_per_pixel
    std::uint32_t green_mask;
    std::uint32_t blue_mask;
    bool msb_first; // byte 0 of a pixel holds its most significant byte
};

// True if convert_pixels can produce this layout: 16/24/32 bpp, three non-empty, contiguous,
// non-overlapping masks of at most 16 bits each, all inside the pixel.
bool pixel_format_supported(const PixelFormat& format);

// Bytes in a row of `width` pixels, unpadded; 0 for an unsupported format.
std::size_t pixel_row_bytes(const PixelFormat& format, int width);

// Writes fb into dst, `stride` bytes per row (stride >= pixel_row_bytes, extra bytes are left
// untouched). dst must hold stride * fb.height() bytes. Returns false, writing nothing, for an
// unsupported format, a null dst or a too-small stride.
bool convert_pixels(const Framebuffer& fb, const PixelFormat& format, std::uint8_t* dst,
                    std::size_t stride);

// Reference converter without layout fast paths, for byte-for-byte checks and measurements.
// Same validation and destination contract as convert_pixels.
bool convert_pixels_generic(const Framebuffer& fb, const PixelFormat& format, std::uint8_t* dst,
                            std::size_t stride);

} // namespace globe
