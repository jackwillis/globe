// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/places.hpp"
#include <algorithm>
#include <cmath>
namespace globe {
void PlaceScene::draw(Framebuffer& fb, const Camera& camera) const {
    for (const auto world : points_) {
        const auto v = to_camera_space(camera, world);
        if (v.z < 0)
            continue;
        const auto p = to_screen(camera, v);
        if (p.x < -3 || p.x > fb.width() + 3 || p.y < -3 || p.y > fb.height() + 3)
            continue;
        const auto x = static_cast<int>(std::floor(p.x + 0.5)),
                   y = static_cast<int>(std::floor(p.y + 0.5));
        for (int row = y - 2; row <= y + 2; ++row)
            fb.fill_span(row, x - 2, x + 2, 0);
        for (int row = y - 1; row <= y + 1; ++row)
            fb.fill_span(row, x - 1, x + 1, 0xffff);
    }
}
void PlaceScene::dot_boxes(const Camera& camera, std::vector<PlacedLabel>& out) const {
    out.clear();
    for (std::size_t i = 0; i < points_.size(); ++i) {
        const auto v = to_camera_space(camera, points_[i]);
        if (v.z < 0)
            continue;
        const auto p = to_screen(camera, v);
        if (p.x < -3 || p.x > camera.width + 3 || p.y < -3 || p.y > camera.height + 3)
            continue;
        const auto x = static_cast<int>(std::floor(p.x + 0.5)),
                   y = static_cast<int>(std::floor(p.y + 0.5));
        out.push_back({i, 0, 0, {x - 3, y - 3, x + 4, y + 4}});
    }
}
std::uint8_t max_place_tier(const Camera& camera) {
    // Radius as a fraction of the shorter window side: a whole globe is about half, a close view
    // several times that.
    const double side = camera.width < camera.height ? camera.width : camera.height;
    const double zoom = camera.radius_px / side;
    if (zoom < 0.8)
        return 0;
    if (zoom < 1.4)
        return 1;
    if (zoom < 2.6)
        return 2;
    return kLastPlaceTier;
}
void select_place_labels(const Places& places, std::span<const LabelCandidate> all,
                         const Camera& camera, std::vector<LabelCandidate>& out,
                         std::uint8_t first_tier, std::uint8_t last_tier) {
    out.clear();
    const auto limit = std::min(max_place_tier(camera), last_tier);
    for (std::uint32_t i = 0; i < all.size() && i < places.count(); ++i)
        if (places.tier(i) >= first_tier && places.tier(i) <= limit &&
            out.size() < kMaxLabelCandidates)
            out.push_back(all[i]);
}
} // namespace globe
