// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include "globe/countries.hpp"
#include "globe/framebuffer.hpp"
#include "globe/projection.hpp"
namespace globe {
class Places {
  public:
    static std::optional<Places> parse(const std::uint8_t* data, std::size_t size);
    std::uint32_t count() const { return count_; }
    std::uint32_t qid(std::uint32_t id) const;
    LatLon point(std::uint32_t id) const;
    std::string_view name(std::uint32_t id) const;

  private:
    const std::uint8_t* data_ = nullptr;
    std::uint32_t count_ = 0;
    std::size_t text_offset_ = 0;
};
class PlaceScene {
  public:
    static std::optional<PlaceScene> create(Places data);
    void draw(Framebuffer& fb, const Camera& camera) const;
    std::optional<std::uint32_t> pick(const Camera& camera, ScreenPoint pointer,
                                      double radius_px = 6) const;

  private:
    Places data_;
    std::vector<Vec3> points_;
};
InfoCard place_card(const Places& places, std::uint32_t id);
} // namespace globe
