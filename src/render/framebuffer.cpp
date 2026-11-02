// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/framebuffer.hpp"

#include <algorithm>
#include <new>

namespace globe {

std::optional<Framebuffer> Framebuffer::create(int width, int height) {
    if (width < 1 || height < 1 || width > kMaxDimension || height > kMaxDimension) {
        return std::nullopt;
    }
    const std::size_t count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    std::unique_ptr<Rgb565[]> pixels(new (std::nothrow) Rgb565[count]());
    if (!pixels) {
        return std::nullopt;
    }
    return Framebuffer(width, height, std::move(pixels));
}

void Framebuffer::clear(Rgb565 color) {
    std::fill_n(pixels_.get(),
                static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_), color);
}

void Framebuffer::set_pixel(int x, int y, Rgb565 color) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        return;
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
    if (y < 0 || y >= height_) {
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
    std::fill(pixels_.get() + static_cast<std::size_t>(y) * static_cast<std::size_t>(stride()) +
                  static_cast<std::size_t>(x0),
              pixels_.get() + static_cast<std::size_t>(y) * static_cast<std::size_t>(stride()) +
                  static_cast<std::size_t>(x1) + 1,
              color);
}

} // namespace globe
