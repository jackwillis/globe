// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "pixel_format.hpp"

#include <cstring>
#include <memory>

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

// The aligned path uses memcpy into known-aligned storage: a native word store without
// violating aliasing. Unaligned destinations use byte stores, never a cast to uint32_t*.
template <bool MsbFirst, bool Aligned>
void pack32_row(const Rgb565* src, std::uint8_t* out, int width, const std::uint32_t* red,
                const std::uint32_t* green, const std::uint32_t* blue) {
    const std::uint16_t endian_probe = 1;
    const bool native_msb = *reinterpret_cast<const std::uint8_t*>(&endian_probe) == 0;
    std::uint32_t previous = 0x10000U; // outside RGB565: first pixel always computes its color
    std::uint32_t packed = 0;
    std::uint32_t native = 0;
    for (int x = 0; x < width; ++x) {
        const std::uint32_t p = src[x];
        if (p != previous) {
            packed = red[p >> 11] | green[(p >> 5) & 63U] | blue[p & 31U];
            previous = p;
            if constexpr (Aligned) {
                native = native_msb == MsbFirst ? packed
                                                : (packed >> 24) | ((packed >> 8) & 0xFF00U) |
                                                      ((packed << 8) & 0xFF0000U) | (packed << 24);
            }
        }
        if constexpr (Aligned) {
            std::memcpy(std::assume_aligned<alignof(std::uint32_t)>(out), &native, sizeof native);
        } else {
            out[0] = static_cast<std::uint8_t>(packed >> (MsbFirst ? 24 : 0));
            out[1] = static_cast<std::uint8_t>(packed >> (MsbFirst ? 16 : 8));
            out[2] = static_cast<std::uint8_t>(packed >> (MsbFirst ? 8 : 16));
            out[3] = static_cast<std::uint8_t>(packed >> (MsbFirst ? 0 : 24));
        }
        out += 4;
    }
}

template <bool MsbFirst>
void pack32(const Framebuffer& fb, std::uint8_t* dst, std::size_t stride, const std::uint32_t* red,
            const std::uint32_t* green, const std::uint32_t* blue) {
    for (int y = 0; y < fb.height(); ++y) {
        const Rgb565* src = fb.data() + static_cast<std::size_t>(y) * fb.stride();
        std::uint8_t* out = dst + static_cast<std::size_t>(y) * stride;
        if (reinterpret_cast<std::uintptr_t>(out) % alignof(std::uint32_t) == 0) {
            pack32_row<MsbFirst, true>(src, out, fb.width(), red, green, blue);
        } else {
            pack32_row<MsbFirst, false>(src, out, fb.width(), red, green, blue);
        }
    }
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

static bool convert_pixels_impl(const Framebuffer& fb, const PixelFormat& format, std::uint8_t* dst,
                                std::size_t stride, bool fast) {
    if (dst == nullptr || !pixel_format_supported(format) ||
        stride < pixel_row_bytes(format, fb.width())) {
        return false;
    }
    if (fast && format.bits_per_pixel == 16 && format.red_mask == 0xF800U &&
        format.green_mask == 0x07E0U && format.blue_mask == 0x001FU) {
        const std::uint16_t endian_probe = 1;
        const bool native_msb = *reinterpret_cast<const std::uint8_t*>(&endian_probe) == 0;
        for (int y = 0; y < fb.height(); ++y) {
            const Rgb565* src = fb.data() + static_cast<std::size_t>(y) * fb.stride();
            std::uint8_t* out = dst + static_cast<std::size_t>(y) * stride;
            if (format.msb_first == native_msb) {
                std::memcpy(out, src, static_cast<std::size_t>(fb.width()) * sizeof(Rgb565));
            } else {
                for (int x = 0; x < fb.width(); ++x) {
                    out[0] = static_cast<std::uint8_t>(src[x] >> (format.msb_first ? 8 : 0));
                    out[1] = static_cast<std::uint8_t>(src[x] >> (format.msb_first ? 0 : 8));
                    out += 2;
                }
            }
        }
        return true;
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
    if (fast && format.bits_per_pixel == 32) {
        if (format.msb_first) {
            pack32<true>(fb, dst, stride, red, green, blue);
        } else {
            pack32<false>(fb, dst, stride, red, green, blue);
        }
        return true;
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

bool convert_pixels(const Framebuffer& fb, const PixelFormat& format, std::uint8_t* dst,
                    std::size_t stride) {
    return convert_pixels_impl(fb, format, dst, stride, true);
}

bool convert_pixels_generic(const Framebuffer& fb, const PixelFormat& format, std::uint8_t* dst,
                            std::size_t stride) {
    return convert_pixels_impl(fb, format, dst, stride, false);
}

} // namespace globe
