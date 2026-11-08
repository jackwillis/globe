// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/framebuffer.hpp"

#include <algorithm>
#include <new>

namespace globe {

std::optional<Framebuffer> Framebuffer::create(int width, int height, MemoryLedger* ledger,
                                               MemoryCategory category) {
    if (width < 1 || height < 1 || width > kMaxDimension || height > kMaxDimension) {
        return std::nullopt;
    }
    const std::size_t count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    MemoryCharge pixels_charge, coverage_charge;
    std::unique_ptr<Rgb565[]> pixels(new (std::nothrow) Rgb565[count]());
    if (!pixels) {
        if (ledger != nullptr)
            ledger->allocation_failed(category);
        return std::nullopt;
    }
    if (ledger != nullptr)
        pixels_charge = MemoryCharge(*ledger, category, count * sizeof(Rgb565));
    const auto coverage_bytes =
        static_cast<std::size_t>((width + 7) / 8) * static_cast<std::size_t>(height);
    std::unique_ptr<std::uint8_t[]> coverage(new (std::nothrow) std::uint8_t[coverage_bytes]());
    if (!coverage) {
        if (ledger != nullptr)
            ledger->allocation_failed(category);
        return std::nullopt;
    }
    if (ledger != nullptr)
        coverage_charge = MemoryCharge(*ledger, category, coverage_bytes);
    return Framebuffer(width, height, std::move(pixels), std::move(coverage),
                       std::move(pixels_charge), std::move(coverage_charge));
}

void Framebuffer::clip_rows(int first, int end) {
    clip_first_ = std::clamp(first, 0, height_);
    clip_end_ = std::clamp(end, clip_first_, height_);
}

void Framebuffer::reset_coverage_row(int y) {
    if (y < 0 || y >= height_)
        return;
    const auto stride = static_cast<std::size_t>((width_ + 7) / 8);
    std::fill(coverage_.get() + y * stride, coverage_.get() + (y + 1) * stride, 0);
}
void Framebuffer::mark_span(int y, int first, int last) {
    if (coverage_mode_ != Coverage::Mark || y < clip_first_ || y >= clip_end_)
        return;
    first = std::max(0, first);
    last = std::min(width_ - 1, last);
    auto* row = coverage_.get() + static_cast<std::size_t>(y) * ((width_ + 7) / 8);
    for (int x = first; x <= last; ++x)
        row[x / 8] |= static_cast<std::uint8_t>(1U << (x % 8));
}
void Framebuffer::clear(Rgb565 color) {
    std::fill(
        pixels_.get() + static_cast<std::size_t>(clip_first_) * static_cast<std::size_t>(stride()),
        pixels_.get() + static_cast<std::size_t>(clip_end_) * static_cast<std::size_t>(stride()),
        color);
}

void Framebuffer::set_pixel(int x, int y, Rgb565 color) {
    if (x < 0 || y < clip_first_ || x >= width_ || y >= clip_end_) {
        return;
    }
    if (coverage_mode_ != Coverage::Off) {
        const auto mask = static_cast<std::uint8_t>(1U << (x % 8));
        auto& coverage = coverage_[static_cast<std::size_t>(y) * ((width_ + 7) / 8) + x / 8];
        if (coverage_mode_ == Coverage::Uncovered && (coverage & mask))
            return;
        if (coverage_mode_ == Coverage::Mark)
            coverage |= mask;
    }
    pixels_[static_cast<std::size_t>(y) * static_cast<std::size_t>(stride()) +
            static_cast<std::size_t>(x)] = color;
}

Rgb565 Framebuffer::pixel(int x, int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        return 0;
    }
    return pixels_[static_cast<std::size_t>(y) * static_cast<std::size_t>(stride()) +
                   static_cast<std::size_t>(x)];
}

void Framebuffer::fill_span(int y, int x0, int x1, Rgb565 color) {
    if (y < clip_first_ || y >= clip_end_) {
        return;
    }
    if (x0 > x1) {
        std::swap(x0, x1);
    }
    x0 = std::max(x0, 0);
    x1 = std::min(x1, width_ - 1);
    if (x0 > x1) {
        return;
    }
    if (coverage_mode_ == Coverage::Uncovered) {
        for (int x = x0; x <= x1; ++x)
            set_pixel(x, y, color);
        return;
    }
    mark_span(y, x0, x1);
    std::fill(pixels_.get() + static_cast<std::size_t>(y) * static_cast<std::size_t>(stride()) +
                  static_cast<std::size_t>(x0),
              pixels_.get() + static_cast<std::size_t>(y) * static_cast<std::size_t>(stride()) +
                  static_cast<std::size_t>(x1) + 1,
              color);
}

} // namespace globe
