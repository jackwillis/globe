// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
namespace globe {
constexpr std::size_t kElevationHeaderBytes = 40;
constexpr std::size_t kElevationCellsOffset = 552;
constexpr std::uint32_t kElevationMaxCells = 16U * 1024U * 1024U;
enum class ElevationError : std::uint8_t {
    Ok,
    TooShort,
    BadHeader,
    TooLarge,
    SizeMismatch,
    BadChecksum,
    BadTable
};
// Strict zero-copy GELV v1 view. Caller-owned bytes must outlive this view and any caches.
// Rows run south to north; longitude runs west to east. Cell extents are stored in microdegrees.
class Elevation {
  public:
    static std::optional<Elevation> parse(const std::uint8_t* data, std::size_t size,
                                          ElevationError* error = nullptr);
    std::uint32_t width() const { return width_; }
    std::uint32_t height() const { return height_; }
    double west_deg() const;
    double east_deg() const;
    double south_deg() const;
    double north_deg() const;
    const std::uint8_t* cells() const { return data_ + kElevationCellsOffset; }
    std::uint8_t index(std::uint32_t x, std::uint32_t y) const;
    std::int16_t value(std::uint8_t index) const;
    std::int16_t meters(std::uint32_t x, std::uint32_t y) const;

  private:
    Elevation(const std::uint8_t* data, std::uint32_t width, std::uint32_t height)
        : data_(data), width_(width), height_(height) {}
    const std::uint8_t* data_;
    std::uint32_t width_, height_;
};
} // namespace globe
