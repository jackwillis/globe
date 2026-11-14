// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/label_layout.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace globe {
namespace {

bool overlaps(LabelBox a, LabelBox b) {
    return a.left < b.right && b.left < a.right && a.top < b.bottom && b.top < a.bottom;
}

bool fits(const Camera& camera, LabelBox box) {
    if (box.left < 0 || box.top < 0 || box.right > camera.width || box.bottom > camera.height) {
        return false;
    }
    // The farthest corner combines the farthest x and y edges. Normalize before squaring.
    const double dx =
        std::max(std::abs(box.left - camera.center_x), std::abs(box.right - camera.center_x)) /
        camera.radius_px;
    const double dy =
        std::max(std::abs(box.top - camera.center_y), std::abs(box.bottom - camera.center_y)) /
        camera.radius_px;
    return dx * dx + dy * dy <= 1.0;
}

bool valid_metrics(LabelMetrics metrics) {
    constexpr int limit = kMaxViewportDimension;
    return metrics.left >= -limit && metrics.right <= limit && metrics.left < metrics.right &&
           metrics.ascent >= 0 && metrics.ascent <= limit && metrics.descent >= 0 &&
           metrics.descent <= limit && metrics.ascent + metrics.descent > 0;
}

} // namespace

std::size_t place_labels(const Camera& camera, std::span<const LabelCandidate> candidates,
                         MeasureLabel measure, void* context, std::span<PlacedLabel> output) {
    const auto cap = std::min(output.size(), kMaxPlacedLabels);
    if (cap == 0 || !measure || candidates.size() > kMaxLabelCandidates) {
        return 0;
    }

    std::array<std::uint16_t, kMaxLabelCandidates> order;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        order[i] = static_cast<std::uint16_t>(i);
    }
    const auto end = order.begin() + static_cast<std::ptrdiff_t>(candidates.size());
    std::sort(order.begin(), end, [candidates](auto a, auto b) {
        const int pa = candidates[a].priority;
        const int pb = candidates[b].priority;
        return pa == pb ? a < b : pa > pb;
    });

    std::size_t count = 0;
    for (auto index = order.begin(); index != end && count < cap; ++index) {
        const auto& label = candidates[*index];
        const Vec3 anchor = to_camera_space(camera, label.anchor);
        const ScreenPoint point = to_screen(camera, anchor);
        if (!is_front(anchor) || label.text.empty() ||
            !(point.x >= 0 && point.x < camera.width && point.y >= 0 && point.y < camera.height)) {
            continue;
        }

        const auto metrics = measure(context, label.text, label.role);
        if (!metrics || !valid_metrics(*metrics)) {
            continue;
        }

        const int width = metrics->right - metrics->left;
        const int height = metrics->ascent + metrics->descent;
        const int left = static_cast<int>(point.x) - width / 2;
        const int top = static_cast<int>(point.y) - height / 2;
        const LabelBox box{left, top, left + width, top + height};
        if (!fits(camera, box) ||
            std::any_of(output.begin(), output.begin() + static_cast<std::ptrdiff_t>(count),
                        [box](const PlacedLabel& placed) { return overlaps(box, placed.box); })) {
            continue;
        }
        output[count++] = {*index, left - metrics->left, top + metrics->ascent, box};
    }
    return count;
}

} // namespace globe
