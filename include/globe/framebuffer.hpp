// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Tiny RGB565 framebuffer: one allocation made up front, bounds-checked drawing, no hidden
// growth. How the pixels reach a window (byte order, depth conversion, QuickDraw layout) is the
// platform adapter's job; this type only owns native-endian 16-bit pixel values.
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

namespace globe {

using Rgb565 = std::uint16_t;

constexpr Rgb565 rgb565(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    return static_cast<Rgb565>(((r & 0xF8U) << 8) | ((g & 0xFCU) << 3) | (b >> 3));
}

class Framebuffer {
  public:
    static constexpr int kMaxDimension = 4096;

    // Fails (nullopt) for a dimension outside 1..kMaxDimension or if allocation fails.
    static std::optional<Framebuffer> create(int width, int height);

    Framebuffer(Framebuffer&&) noexcept = default;
    Framebuffer& operator=(Framebuffer&&) noexcept = default;
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    ~Framebuffer() = default;

    int width() const { return width_; }
    int height() const { return height_; }
    // Row pitch in pixels. Equal to width() today; callers must not assume it.
    int stride() const { return width_; }
    std::size_t byte_size() const {
        return static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_) *
               sizeof(Rgb565);
    }

    Rgb565* data() { return pixels_.get(); }
    const Rgb565* data() const { return pixels_.get(); }

    void clear(Rgb565 color);
    // Out-of-range coordinates are ignored.
    void set_pixel(int x, int y, Rgb565 color);
    // Returns 0 for out-of-range coordinates.
    Rgb565 pixel(int x, int y) const;
    // Fills row y from x0 to x1 inclusive in either order, clipped to the buffer.
    void fill_span(int y, int x0, int x1, Rgb565 color);

  private:
    Framebuffer(int width, int height, std::unique_ptr<Rgb565[]> pixels)
        : width_(width), height_(height), pixels_(std::move(pixels)) {}

    int width_;
    int height_;
    std::unique_ptr<Rgb565[]> pixels_;
};

} // namespace globe
