// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include <algorithm>
#include <cstdint>
namespace globe {
struct ClassicWindowSize {
    int width, height;
};
// The smallest window a user can work in: a 256-pixel globe pane beside the sidebar, 256 tall.
constexpr int kClassicMinGlobeWidth = 256, kClassicMinHeight = 256;
// Best-effort startup hint, not a reservation or a promise that allocations will succeed.
// Only the globe pane owns pixel buffers; the fixed sidebar is excluded. FreeMem reports total
// current-zone free bytes, not contiguous or potentially growable space: the caller must first
// extend the zone to the partition (MaxApplZone), or this reports only the zone's first extent.
// Later resize peaks can retain old and new storage and are outside this initial-size estimate.
constexpr ClassicWindowSize fit_classic_window_to_heap(int width, int height, int sidebar_width,
                                                       std::int64_t free_bytes) {
    constexpr std::int64_t kBytesPerPixel = 11, kHeadroom = 1024 * 1024;
    // Bound all geometry before multiplication, including defensive calls outside OS9's Rect API.
    const int sidebar = std::clamp(sidebar_width, 0, 1440);
    const int min_width = sidebar + kClassicMinGlobeWidth;
    width = std::clamp(width, min_width, 1600);
    height = std::clamp(height, kClassicMinHeight, 1200);
    const auto budget = free_bytes > kHeadroom ? (free_bytes - kHeadroom) / kBytesPerPixel : 0;
    while (static_cast<std::int64_t>(width - sidebar) * height > budget &&
           (width > min_width || height > kClassicMinHeight)) {
        width = std::max(min_width, width * 7 / 8);
        height = std::max(kClassicMinHeight, height * 7 / 8);
    }
    return {width, height}; // minimum may still exceed an exhausted budget
}
// #116: a grow-box enlargement is refused only when the larger buffers cannot be allocated, which
// on Classic means the application partition is too small. The first such refusal in a session
// is worth explaining; later ones (and ordinary size limits, which never reach the allocation)
// are not. The caller shows the alert after GrowWindow has returned, never inside the drag.
class ClassicMemoryRefusalNotice {
  public:
    // A refused shrink (the new buffers are built before the old are freed, so it can fail too)
    // is not explained: raising the partition is not the remedy the message offers. It does not
    // use up the session's one explanation either.
    constexpr bool should_explain(int old_width, int old_height, int new_width, int new_height) {
        const bool enlarging = new_width > old_width || new_height > old_height;
        const bool first = enlarging && !explained_;
        explained_ = explained_ || enlarging;
        return first;
    }

  private:
    bool explained_ = false;
};
} // namespace globe
