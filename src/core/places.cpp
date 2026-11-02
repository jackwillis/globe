// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/places.hpp"
#include <cmath>
namespace globe {
std::optional<PlaceScene> PlaceScene::create(Places data) {
    PlaceScene scene;
    scene.data_ = data;
    scene.points_.reserve(data.count());
    for (std::uint32_t i = 0; i < data.count(); ++i)
        scene.points_.push_back(to_unit_vector(data.point(i)));
    return scene;
}
std::optional<std::uint32_t> PlaceScene::pick(const Camera& camera, ScreenPoint pointer,
                                              double radius) const {
    if (!std::isfinite(pointer.x) || !std::isfinite(pointer.y) || !std::isfinite(radius) ||
        radius < 0)
        return std::nullopt;
    double best = radius * radius;
    std::optional<std::uint32_t> found;
    for (std::uint32_t i = 0; i < data_.count(); ++i) {
        const auto v = to_camera_space(camera, points_[i]);
        if (v.z < 0)
            continue;
        const auto p = to_screen(camera, v);
        const double x = p.x - pointer.x, y = p.y - pointer.y, d = x * x + y * y;
        if (d <= best && (!found || d < best)) {
            found = i;
            best = d;
        }
    }
    return found;
}
InfoCard place_card(const Places& data, std::uint32_t id) {
    InfoCard card;
    card.title = data.name(id);
    if (card.title.empty())
        return card;
    card.lines = {"Selected city / Wikidata", "Coordinates: 2026-11-01 snapshot",
                  "CC0 structured geographic data", "Small starter set; not all cities.",
                  "wikidata.org"};
    card.line_count = 5;
    return card;
}
} // namespace globe
