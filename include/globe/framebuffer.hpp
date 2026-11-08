// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Tiny RGB565 framebuffer: pixels and a coverage bitplane allocated up front, no hidden
// growth. How the pixels reach a window (byte order, depth conversion, QuickDraw layout) is the
// platform adapter's job; this type only owns native-endian 16-bit pixel values.
#pragma once

#include "globe/memory_ledger.hpp"

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
    // Optional accounting borrows a stable ledger which must outlive this framebuffer. Passing
    // nullptr preserves uninstrumented behavior; category distinguishes primary/line workspaces.
    static std::optional<Framebuffer> create(int width, int height, MemoryLedger* ledger = nullptr,
                                             MemoryCategory category = MemoryCategory::Framebuffer);

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

    std::size_t coverage_byte_size() const {
        return static_cast<std::size_t>((width_ + 7) / 8) * height_;
    }

    Rgb565* data() { return pixels_.get(); }
    const Rgb565* data() const { return pixels_.get(); }

    // Drawing clip, half-open rows; reads and dimensions retain full framebuffer coordinates.
    void clip_rows(int first, int end);
    int first_row() const { return clip_first_; }
    int end_row() const { return clip_end_; }
    enum class Coverage : std::uint8_t { Off, Mark, Uncovered };
    void coverage(Coverage mode) { coverage_mode_ = mode; }
    void reset_coverage_row(int y);
    bool recording_coverage() const { return coverage_mode_ == Coverage::Mark; }
    void mark_span(int y, int first, int last);
    void clear(Rgb565 color);
    // Out-of-range coordinates are ignored.
    void set_pixel(int x, int y, Rgb565 color);
    // Returns 0 for out-of-range coordinates.
    Rgb565 pixel(int x, int y) const;
    // Fills row y from x0 to x1 inclusive in either order, clipped to the buffer.
    void fill_span(int y, int x0, int x1, Rgb565 color);

  private:
    Framebuffer(int width, int height, std::unique_ptr<Rgb565[]> pixels,
                std::unique_ptr<std::uint8_t[]> coverage, MemoryCharge pixels_charge,
                MemoryCharge coverage_charge)
        : width_(width), height_(height), clip_end_(height),
          pixels_charge_(std::move(pixels_charge)), coverage_charge_(std::move(coverage_charge)),
          pixels_(std::move(pixels)), coverage_(std::move(coverage)) {}

    int width_;
    int height_;
    int clip_first_ = 0;
    int clip_end_;
    // Tickets precede payload owners so ordinary destruction frees storage before releasing its
    // accounting. Default moves transfer tickets without reacquiring allocation events.
    MemoryCharge pixels_charge_;
    MemoryCharge coverage_charge_;
    std::unique_ptr<Rgb565[]> pixels_;
    std::unique_ptr<std::uint8_t[]> coverage_;
    Coverage coverage_mode_ = Coverage::Off;
};

} // namespace globe
