// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include "globe/countries.hpp"
#include "globe/framebuffer.hpp"
#include "globe/label_layout.hpp"
#include "globe/projection.hpp"
#include <span>
#include <vector>
namespace globe {
constexpr std::uint8_t kLastPlaceTier = 3;
// Tiers 0 and 1 are the large cities, drawn a point larger than the rest (typography.md).
constexpr std::uint8_t kLastLargeCityTier = 1;
class Places {
  public:
    static std::optional<Places> parse(const std::uint8_t* data, std::size_t size);
    std::uint32_t count() const { return count_; }
    std::uint32_t qid(std::uint32_t id) const;
    LatLon point(std::uint32_t id) const;
    std::string_view name(std::uint32_t id) const;
    // Population tier: 0 is the largest cities, kLastPlaceTier the smallest. GPLC v1 packs carry
    // no tiers, so every place reports the last one.
    std::uint8_t tier(std::uint32_t id) const;
    // One candidate per place, in pack order, for the label placer: beside its dot, priority by
    // tier.
    std::vector<LabelCandidate> label_candidates() const;

  private:
    const std::uint8_t* data_ = nullptr;
    std::uint32_t count_ = 0;
    bool has_tiers_ = false;
    std::size_t text_offset_ = 0;
};
class PlaceScene {
  public:
    static std::optional<PlaceScene> create(Places data);
    void draw(Framebuffer& fb, const Camera& camera) const;
    // Boxes of the dots `draw` paints at this camera, with a pixel around each, as label avoid
    // boxes. Only candidate and box are meaningful (candidate is the place index).
    void dot_boxes(const Camera& camera, std::vector<PlacedLabel>& out) const;
    std::optional<std::uint32_t> pick(const Camera& camera, ScreenPoint pointer,
                                      double radius_px = 6) const;

  private:
    Places data_;
    std::vector<Vec3> points_;
};
// The smallest tier worth naming at this zoom: only the largest cities on a whole globe, more as
// the view closes in.
std::uint8_t max_place_tier(const Camera& camera);
// Candidates for one frame: the entries of `all` (from Places::label_candidates) whose tier suits
// the zoom and lie in first_tier..last_tier, capped at the placer's candidate limit.
void select_place_labels(const Places& places, std::span<const LabelCandidate> all,
                         const Camera& camera, std::vector<LabelCandidate>& out,
                         std::uint8_t first_tier = 0, std::uint8_t last_tier = kLastPlaceTier);
InfoCard place_card(const Places& places, std::uint32_t id);
} // namespace globe
