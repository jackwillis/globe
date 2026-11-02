// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "pixel_format.hpp"

namespace globe {

namespace {

struct Channel {
    int shift;
    int bits;
};

// Contiguous run of set bits -> shift and width; bits == 0 if empty or not contiguous.
Channel analyze(std::uint32_t mask) {
    if (mask == 0) {
        return {0, 0};
    }
    int shift = 0;
    while ((mask & 1U) == 0) {
        mask >>= 1;
        ++shift;
    }
    int bits = 0;
    while ((mask & 1U) != 0) {
        mask >>= 1;
        ++bits;
    }
    return mask == 0 ? Channel{shift, bits} : Channel{0, 0};
}

// Scale a value in [0, in_max] to [0, 2^bits - 1], rounding to nearest, endpoints exact.
std::uint32_t scale(std::uint32_t v, std::uint32_t in_max, int bits) {
    const std::uint32_t out_max = (1U << bits) - 1U;
    return (v * out_max + in_max / 2) / in_max;
}

} // namespace

bool pixel_format_supported(const PixelFormat& f) {
    if (f.bits_per_pixel != 16 && f.bits_per_pixel != 24 && f.bits_per_pixel != 32) {
        return false;
    }
    const Channel r = analyze(f.red_mask);
    const Channel g = analyze(f.green_mask);
    const Channel b = analyze(f.blue_mask);
    if (r.bits == 0 || g.bits == 0 || b.bits == 0 || r.bits > 16 || g.bits > 16 || b.bits > 16) {
        return false;
    }
    if ((f.red_mask & f.green_mask) != 0 || (f.red_mask & f.blue_mask) != 0 ||
        (f.green_mask & f.blue_mask) != 0) {
        return false;
    }
    const std::uint64_t pixel_limit = 1ULL << f.bits_per_pixel;
    return f.red_mask < pixel_limit && f.green_mask < pixel_limit && f.blue_mask < pixel_limit;
}

std::size_t pixel_row_bytes(const PixelFormat& format, int width) {
    if (!pixel_format_supported(format) || width < 0) {
        return 0;
    }
    return static_cast<std::size_t>(width) * static_cast<std::size_t>(format.bits_per_pixel / 8);
}

bool convert_pixels(const Framebuffer& fb, const PixelFormat& format, std::uint8_t* dst,
                    std::size_t stride) {
    if (dst == nullptr || !pixel_format_supported(format) ||
        stride < pixel_row_bytes(format, fb.width())) {
        return false;
    }
    const Channel r = analyze(format.red_mask);
    const Channel g = analyze(format.green_mask);
    const Channel b = analyze(format.blue_mask);
    // One table per source channel value; entries are already shifted into place.
    std::uint32_t red[32];
    std::uint32_t green[64];
    std::uint32_t blue[32];
    for (std::uint32_t i = 0; i < 32; ++i) {
        red[i] = scale(i, 31, r.bits) << r.shift;
        blue[i] = scale(i, 31, b.bits) << b.shift;
    }
    for (std::uint32_t i = 0; i < 64; ++i) {
        green[i] = scale(i, 63, g.bits) << g.shift;
    }
    const int bytes = format.bits_per_pixel / 8;
    for (int y = 0; y < fb.height(); ++y) {
        const Rgb565* src =
            fb.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(fb.stride());
        std::uint8_t* out = dst + static_cast<std::size_t>(y) * stride;
        for (int x = 0; x < fb.width(); ++x) {
            const std::uint32_t p = src[x];
            const std::uint32_t v = red[p >> 11] | green[(p >> 5) & 0x3FU] | blue[p & 0x1FU];
            if (format.msb_first) {
                for (int k = 0; k < bytes; ++k) {
                    out[k] = static_cast<std::uint8_t>(v >> (8 * (bytes - 1 - k)));
                }
            } else {
                for (int k = 0; k < bytes; ++k) {
                    out[k] = static_cast<std::uint8_t>(v >> (8 * k));
                }
            }
            out += bytes;
        }
    }
    return true;
}

} // namespace globe
