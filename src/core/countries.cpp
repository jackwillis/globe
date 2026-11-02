// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/countries.hpp"
#include <cmath>
namespace globe {
std::optional<CountryScene> CountryScene::create(Countries data) {
    CountryScene out;
    out.data_ = data;
    out.points_.reserve(data.point_count());
    for (std::uint32_t i = 0; i < data.point_count(); ++i)
        out.points_.push_back(to_unit_vector(data.point(i)));
    return out;
}
int CountryScene::contains(CountryRing ring, Vec3 p) const {
    double winding = 0;
    auto a = points_[ring.first + ring.count - 1];
    for (std::uint32_t i = 0; i < ring.count; ++i) {
        const auto b = points_[ring.first + i], normal = cross(a, b);
        const double n = length(normal);
        if (n > 1e-14 && std::fabs(dot(p, normal)) <= 1e-10 * n &&
            dot(p, a + b) >= 1 + dot(a, b) - 1e-10)
            return -1;
        winding += std::atan2(dot(p, normal), dot(a, b) - dot(p, a) * dot(p, b));
        a = b;
    }
    return winding > kPi ? 1 : 0;
}
std::optional<std::uint32_t> CountryScene::pick(LatLon point) const {
    if (!std::isfinite(point.lat_deg) || !std::isfinite(point.lon_deg) ||
        std::fabs(point.lat_deg) > 90)
        return std::nullopt;
    const auto p = to_unit_vector(point);
    for (std::uint32_t i = 0; i < data_.count(); ++i) {
        const auto c = data_.country(i);
        for (std::uint32_t j = c.first; j < c.first + c.count; ++j) {
            const auto outer = data_.ring(j);
            if (outer.hole || !contains(outer, p))
                continue;
            bool excluded = false;
            for (std::uint32_t k = c.first; k < c.first + c.count; ++k) {
                const auto hole = data_.ring(k);
                if (hole.hole && hole.parent == j && contains(hole, p)) {
                    excluded = true;
                    break;
                }
            }
            if (!excluded)
                return i;
        }
    }
    return std::nullopt;
}
InfoCard country_card(const Countries& data, std::uint32_t id) {
    InfoCard out;
    out.title = data.name(id);
    if (out.title.empty())
        return out;
    out.lines = {"Country or territory",
                 "Natural Earth 1:110m / 5.1.1",
                 "Public domain geographic data",
                 "Generalized boundaries; islands omitted.",
                 "Names and disputed areas follow",
                 "Natural Earth's source records.",
                 "naturalearthdata.com"};
    out.line_count = 7;
    return out;
}
} // namespace globe
