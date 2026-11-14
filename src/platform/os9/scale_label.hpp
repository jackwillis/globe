// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// The scale's label in Monaco, a monospace system font every Mac OS 9 has, drawn with QuickDraw
// into a one-bit bitmap that the shared scale overlay puts on the map (with its black edge). The
// bitmap is made again only when the text or size changes. Without Monaco the built-in digits are
// used, so `bitmap()` is then null.
#pragma once
#include "globe/scale_overlay.hpp"
#include <cstdint>
namespace globe {
class ScaleLabel {
  public:
    // The label for the camera in a frame `width` wide; call before building each shown frame.
    void sync(const Camera& camera, int width);
    // Null when there is no scale or no Monaco.
    const LabelBitmap* bitmap() const { return ready_ ? &bitmap_ : nullptr; }

  private:
    static constexpr int kMaxWidth = 192; // pixels
    static constexpr int kStride = kMaxWidth / 8;
    std::uint8_t bits_[LabelBitmap::kMaxHeight * kStride] = {};
    LabelBitmap bitmap_{};
    char text_[16] = {};
    short size_ = 0;
    bool ready_ = false;
};
} // namespace globe
