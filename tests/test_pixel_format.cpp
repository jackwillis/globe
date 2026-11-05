// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include <cstdint>
#include <cstring>
#include <vector>

#include "check.hpp"
#include "pixel_format.hpp"

using namespace globe;

namespace {

constexpr PixelFormat k32Le = {32, 0x00FF0000U, 0x0000FF00U, 0x000000FFU, false}; // BGRX in memory
constexpr PixelFormat k32Be = {32, 0x00FF0000U, 0x0000FF00U, 0x000000FFU, true};  // XRGB in memory
constexpr PixelFormat k24Le = {24, 0xFF0000U, 0x00FF00U, 0x0000FFU, false};
constexpr PixelFormat k16_565 = {16, 0xF800U, 0x07E0U, 0x001FU, false};
constexpr PixelFormat k16_565Be = {16, 0xF800U, 0x07E0U, 0x001FU, true};
constexpr PixelFormat k16_555 = {16, 0x7C00U, 0x03E0U, 0x001FU, false};
constexpr PixelFormat kRgbx = {32, 0xFF000000U, 0x00FF0000U, 0x0000FF00U, false}; // channels high

Framebuffer one_row(std::initializer_list<Rgb565> pixels) {
    auto fb = Framebuffer::create(static_cast<int>(pixels.size()), 1);
    CHECK(fb.has_value());
    int x = 0;
    for (const Rgb565 p : pixels) {
        fb->set_pixel(x++, 0, p);
    }
    return std::move(*fb);
}

std::vector<std::uint8_t> convert(const Framebuffer& fb, const PixelFormat& f) {
    std::vector<std::uint8_t> out(
        pixel_row_bytes(f, fb.width()) * static_cast<std::size_t>(fb.height()), 0xEE);
    CHECK(convert_pixels(fb, f, out.data(), pixel_row_bytes(f, fb.width())));
    return out;
}

void test_known_values() {
    const Framebuffer fb = one_row({0x0000, 0xFFFF, 0xF800, 0x07E0, 0x001F});
    // 32 bpp little-endian image order: bytes are B, G, R, pad.
    const std::vector<std::uint8_t> le = convert(fb, k32Le);
    const std::uint8_t want_le[] = {0,    0, 0, 0,    0xFF, 0xFF, 0xFF, 0, 0, 0,
                                    0xFF, 0, 0, 0xFF, 0,    0,    0xFF, 0, 0, 0};
    CHECK(le.size() == sizeof want_le && std::memcmp(le.data(), want_le, sizeof want_le) == 0);
    // Most-significant-byte-first: pad, R, G, B.
    const std::vector<std::uint8_t> be = convert(fb, k32Be);
    const std::uint8_t want_be[] = {0, 0, 0, 0, 0,    0xFF, 0xFF, 0xFF, 0, 0xFF,
                                    0, 0, 0, 0, 0xFF, 0,    0,    0,    0, 0xFF};
    CHECK(be.size() == sizeof want_be && std::memcmp(be.data(), want_be, sizeof want_be) == 0);
    // 24 bpp.
    const std::vector<std::uint8_t> p24 = convert(fb, k24Le);
    const std::uint8_t want24[] = {0, 0, 0, 0xFF, 0xFF, 0xFF, 0, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0};
    CHECK(p24.size() == sizeof want24 && std::memcmp(p24.data(), want24, sizeof want24) == 0);
    // 565 is the identity in either byte order.
    const std::vector<std::uint8_t> id = convert(fb, k16_565);
    const std::vector<std::uint8_t> idb = convert(fb, k16_565Be);
    for (int i = 0; i < 5; ++i) {
        CHECK(id[2 * i] == (fb.pixel(i, 0) & 0xFF) && id[2 * i + 1] == (fb.pixel(i, 0) >> 8));
        CHECK(idb[2 * i] == (fb.pixel(i, 0) >> 8) && idb[2 * i + 1] == (fb.pixel(i, 0) & 0xFF));
    }
    // 555: green drops one bit; full green still maps to full scale.
    const std::vector<std::uint8_t> p555 =
        convert(one_row({0xFFFF, 0x07E0, 0x0020, 0x0040}), k16_555);
    CHECK(p555[0] == 0xFF && p555[1] == 0x7F); // 0x7FFF
    CHECK(p555[2] == 0xE0 && p555[3] == 0x03); // 0x03E0
    CHECK(p555[4] == 0x00 && p555[5] == 0x00); // 6-bit green 1 is 0.49 of a 5-bit step: rounds to 0
    CHECK(p555[6] == 0x20 && p555[7] == 0x00); // 6-bit green 2 is 0.98: rounds to 1 -> 0x0020
    // Channels in the high bits of a 32-bit pixel.
    const std::vector<std::uint8_t> hi = convert(one_row({0xF800}), kRgbx);
    CHECK(hi[3] == 0xFF && hi[2] == 0 && hi[1] == 0 && hi[0] == 0);
}

// Exhaustive: every RGB565 value, against an independent per-channel computation.
void test_exhaustive_8bit_channels() {
    auto fb = Framebuffer::create(256, 256);
    CHECK(fb.has_value());
    if (!fb) {
        return;
    }
    for (int i = 0; i < 65536; ++i) {
        fb->set_pixel(i & 255, i >> 8, static_cast<Rgb565>(i));
    }
    const PixelFormat formats[] = {k32Le, k32Be, k24Le};
    for (const PixelFormat& f : formats) {
        std::vector<std::uint8_t> out(pixel_row_bytes(f, 256) * 256);
        CHECK(convert_pixels(*fb, f, out.data(), pixel_row_bytes(f, 256)));
        const int bytes = f.bits_per_pixel / 8;
        int bad = 0;
        for (int i = 0; i < 65536 && bad < 5; ++i) {
            const unsigned r5 = static_cast<unsigned>(i) >> 11;
            const unsigned g6 = (static_cast<unsigned>(i) >> 5) & 63;
            const unsigned b5 = static_cast<unsigned>(i) & 31;
            const unsigned r8 = (r5 * 255 + 15) / 31;
            const unsigned g8 = (g6 * 255 + 31) / 63;
            const unsigned b8 = (b5 * 255 + 15) / 31;
            const std::uint8_t* p = out.data() + static_cast<std::size_t>(i) * bytes;
            // Within each pixel the byte holding R, G, B depends on order and width.
            unsigned got_r;
            unsigned got_g;
            unsigned got_b;
            if (f.msb_first) {
                got_r = p[bytes - 3];
                got_g = p[bytes - 2];
                got_b = p[bytes - 1];
            } else {
                got_b = p[0];
                got_g = p[1];
                got_r = p[2];
            }
            if (got_r != r8 || got_g != g8 || got_b != b8) {
                ++bad;
            }
        }
        CHECK(bad == 0);
    }
}

void test_stride_and_rows() {
    auto fb = Framebuffer::create(3, 2);
    CHECK(fb.has_value());
    fb->clear(0xFFFF);
    fb->set_pixel(0, 1, 0x0000);
    const std::size_t stride = 16; // wider than 3 * 4 = 12
    std::vector<std::uint8_t> out(stride * 2, 0xAA);
    CHECK(convert_pixels(*fb, k32Le, out.data(), stride));
    CHECK(out[12] == 0xAA && out[15] == 0xAA); // row padding untouched
    CHECK(out[28] == 0xAA && out[31] == 0xAA);
    CHECK(out[0] == 0xFF && out[stride + 0] == 0x00); // (0,1) is black
    CHECK(out[stride + 4] == 0xFF);                   // (1,1) white
    // Too-small stride and null destination are refused without writing.
    std::vector<std::uint8_t> small(64, 0x55);
    CHECK(!convert_pixels(*fb, k32Le, small.data(), 11));
    CHECK(!convert_pixels(*fb, k32Le, nullptr, 64));
    for (const std::uint8_t b : small) {
        CHECK(b == 0x55);
    }
}

void test_fast_matches_generic() {
    // Odd width, unaligned destination and padded rows exercise tails and alignment; the
    // sequential pattern covers all 65,536 inputs, then flat runs exercise cached colors.
    auto fb = Framebuffer::create(257, 257);
    CHECK(fb.has_value());
    if (!fb) {
        return;
    }
    const PixelFormat formats[] = {k16_555,
                                   {16, 0x7C00U, 0x03E0U, 0x001FU, true},
                                   k32Le,
                                   k32Be,
                                   kRgbx,
                                   k16_565,
                                   k16_565Be,
                                   k24Le,
                                   {32, 0x3FF00000U, 0x000FFC00U, 0x000003FFU, true}};
    for (int pattern = 0; pattern < 2; ++pattern) {
        for (int y = 0; y < fb->height(); ++y) {
            for (int x = 0; x < fb->width(); ++x) {
                const unsigned i = static_cast<unsigned>(y * fb->width() + x);
                fb->set_pixel(x, y, static_cast<Rgb565>(pattern == 0 ? i : i / 71));
            }
        }
        for (const PixelFormat& f : formats) {
            const std::size_t stride = pixel_row_bytes(f, fb->width()) + 7;
            std::vector<std::uint8_t> fast(stride * 257 + 2, 0xA5);
            std::vector<std::uint8_t> generic(fast.size(), 0xA5);
            CHECK(convert_pixels(*fb, f, fast.data() + 1, stride));
            CHECK(convert_pixels_generic(*fb, f, generic.data() + 1, stride));
            CHECK(fast == generic); // includes every row's padding and both guard bytes
        }
    }
    std::uint8_t unchanged[32];
    std::memset(unchanged, 0xA5, sizeof unchanged);
    CHECK(!convert_pixels_generic(*fb, k32Le, unchanged, 1));
    CHECK(!convert_pixels_generic(*fb, k32Le, nullptr, 4096));
    CHECK(!convert_pixels_generic(*fb, {8, 0xE0, 0x1C, 3, false}, unchanged, 4096));
    for (std::uint8_t byte : unchanged) {
        CHECK(byte == 0xA5);
    }
}

void test_row_ranges() {
    auto fb = Framebuffer::create(7, 5);
    CHECK(fb.has_value());
    for (int y = 0; y < 5; ++y)
        for (int x = 0; x < 7; ++x)
            fb->set_pixel(x, y, static_cast<Rgb565>(y * 9381 + x * 177));
    for (const auto f : {k16_555, PixelFormat{16, 0x7C00U, 0x03E0U, 0x001FU, true}, k16_565, k32Le,
                         k32Be, k24Le}) {
        const auto row = pixel_row_bytes(f, 7);
        const auto stride = row + 3;
        std::vector<std::uint8_t> reference(stride * 5 + 2, 0xA5);
        CHECK(convert_pixels_generic(*fb, f, reference.data() + 1, stride));
        auto out = std::vector<std::uint8_t>(reference.size(), 0xA5);
        CHECK(convert_pixel_rows(*fb, f, out.data() + 1, stride, 1, 4));
        for (std::size_t i = 0; i < out.size(); ++i) {
            const bool touched = i >= 1 + stride && i < 1 + 4 * stride && (i - 1) % stride < row;
            CHECK(out[i] == (touched ? reference[i] : 0xA5));
        }
        const auto saved = out;
        CHECK(convert_pixel_rows(*fb, f, out.data() + 1, stride, 5, 5));
        CHECK(!convert_pixel_rows(*fb, f, out.data() + 1, stride, -1, 4));
        CHECK(!convert_pixel_rows(*fb, f, out.data() + 1, stride, 3, 2));
        CHECK(!convert_pixel_rows(*fb, f, out.data() + 1, stride, 0, 6));
        CHECK(out == saved);
    }
}

void test_unsupported_formats() {
    CHECK(pixel_format_supported(k32Le) && pixel_format_supported(k16_555));
    CHECK(!pixel_format_supported({8, 0xE0, 0x1C, 0x03, false}));              // 8 bpp (palette)
    CHECK(!pixel_format_supported({15, 0x7C00, 0x03E0, 0x001F, false}));       // odd bpp
    CHECK(!pixel_format_supported({32, 0, 0xFF00, 0xFF, false}));              // empty mask
    CHECK(!pixel_format_supported({32, 0xFF00FF, 0xFF00, 0, false}));          // empty + split
    CHECK(!pixel_format_supported({32, 0xF0F000, 0x0000FF, 0x00F, false}));    // split + overlap
    CHECK(!pixel_format_supported({32, 0xFF0000, 0x00FF00, 0x00FF80, false})); // overlap
    CHECK(!pixel_format_supported({32, 0xFF0000, 0xFF0000, 0x0000FF, false})); // red = green
    CHECK(!pixel_format_supported({32, 0xFF0000, 0x00FF00, 0xFF0000, false})); // red = blue
    CHECK(!pixel_format_supported({32, 0xF000F0, 0x000F00, 0x00000F, false})); // red is split
    CHECK(!pixel_format_supported({32, 0x00F000, 0x0F00F0, 0x00000F, false})); // green is split
    CHECK(!pixel_format_supported({32, 0xF00000, 0x000F00, 0x0F000F, false})); // blue is split
    CHECK(!pixel_format_supported({16, 0xF800, 0x07E0, 0x1001F, false}));      // beyond 16 bits
    CHECK(!pixel_format_supported({32, 0x1FFFFFF, 0, 0, false})); // over 16-bit channel
    CHECK(!pixel_format_supported({24, 0xFF000000U, 0x00FF00, 0x0000FF, false})); // beyond 24 bits
    CHECK(pixel_row_bytes({8, 0xE0, 0x1C, 0x03, false}, 10) == 0);
    CHECK(pixel_row_bytes(k24Le, 10) == 30 && pixel_row_bytes(k16_565, 10) == 20);
    CHECK(pixel_row_bytes(k32Le, -1) == 0);
    auto fb = Framebuffer::create(2, 2);
    std::uint8_t buf[64] = {};
    CHECK(!convert_pixels(*fb, {8, 0xE0, 0x1C, 0x03, false}, buf, sizeof buf));
}

} // namespace

int main() {
    test_row_ranges();
    test_fast_matches_generic();
    test_known_values();
    test_exhaustive_8bit_channels();
    test_stride_and_rows();
    test_unsupported_formats();
    return TEST_MAIN_RESULT();
}
