// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
namespace globe {
enum class LandClass : std::uint8_t { Unknown, Desert, Forest, GrassShrub, Tundra };
constexpr unsigned kLandClassCount = 5;
// Typed GGRD v1: exactly 360x180, global one-degree WGS84, south-first. Values 0..4 only.
// Decoded once at startup; no hidden decode/allocation during rendering.
class LandClasses {
  public:
    static std::optional<LandClasses> decode(const std::uint8_t* data, std::size_t size);
    LandClass at(double latitude, double longitude) const;
    static constexpr std::size_t bytes() { return std::size_t{360} * 180; }

  private:
    explicit LandClasses(std::unique_ptr<std::uint8_t[]> cells) : cells_(std::move(cells)) {}
    std::unique_ptr<std::uint8_t[]> cells_;
};
} // namespace globe
