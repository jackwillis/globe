// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include <algorithm>
#include <cstdint>
namespace globe {
struct ClassicWindowSize {
    int width, height;
};
// Best-effort startup hint, not a reservation or a promise that allocations will succeed.
// Only the globe pane owns pixel buffers; the fixed sidebar is excluded. FreeMem reports total
// current-zone free bytes, not contiguous or potentially growable space. Later resize peaks can
// retain old and new storage and are outside this initial-size estimate.
constexpr ClassicWindowSize fit_classic_window_to_heap(int width, int height, int sidebar_width,
                                                       std::int64_t free_bytes) {
    constexpr std::int64_t kBytesPerPixel = 11, kHeadroom = 1024 * 1024;
    // Bound all geometry before multiplication, including defensive calls outside OS9's Rect API.
    const int sidebar = std::clamp(sidebar_width, 0, 1440);
    const int min_width = sidebar + 160;
    width = std::clamp(width, min_width, 1600);
    height = std::clamp(height, 120, 1200);
    const auto budget = free_bytes > kHeadroom ? (free_bytes - kHeadroom) / kBytesPerPixel : 0;
    while (static_cast<std::int64_t>(width - sidebar) * height > budget &&
           (width > min_width || height > 120)) {
        width = std::max(min_width, width * 7 / 8);
        height = std::max(120, height * 7 / 8);
    }
    return {width, height}; // minimum may still exceed an exhausted budget
}
} // namespace globe
