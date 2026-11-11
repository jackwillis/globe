// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include "globe/geometry.hpp"
#include "globe/strings.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>
namespace globe {
struct InfoCard {
    std::string_view title;
    std::array<std::string_view, 12> lines{};
    unsigned line_count = 0;
};
struct CountryRecord {
    std::uint32_t first, count;
};
struct CountryRing {
    std::uint32_t first, count, hole, parent;
};
// Zero-copy immutable view. The input bytes must outlive this view and its scene.
class Countries {
  public:
    static std::optional<Countries> parse(const std::uint8_t* bytes, std::size_t size);
    std::uint32_t count() const { return count_; }
    std::uint32_t point_count() const { return points_; }
    CountryRecord country(std::uint32_t id) const;
    CountryRing ring(std::uint32_t id) const;
    LatLon point(std::uint32_t id) const;
    std::string_view name(std::uint32_t id) const;

  private:
    const std::uint8_t* data_ = nullptr;
    std::uint32_t count_ = 0, rings_ = 0, points_ = 0;
    std::size_t ring_offset_ = 0, point_offset_ = 0, text_offset_ = 0;
};
class CountryScene {
  public:
    static std::optional<CountryScene> create(Countries data);
    std::optional<std::uint32_t> pick(LatLon point) const;

  private:
    int contains(CountryRing ring, Vec3 point) const; // -1 boundary, 0 outside, 1 inside
    Countries data_;
    std::vector<Vec3> points_;
};
InfoCard country_card(const Countries& countries, std::uint32_t id);
// The blank state of the information window: shown at start and when a click clears the selection.
inline InfoCard nothing_selected_card() {
    InfoCard card;
    card.title = tr(Str::Selection);
    card.lines[0] = tr(Str::NothingSelected);
    card.line_count = 1;
    return card;
}
} // namespace globe
