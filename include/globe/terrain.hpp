// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include "globe/elevation.hpp"
#include "globe/framebuffer.hpp"
#include "globe/projection.hpp"
#include "globe/refine_order.hpp"
#include "globe/terrain_style.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
namespace globe {
constexpr std::size_t kTerrainCacheBudget = std::size_t{32} * 1024U * 1024U;
struct CellBands {
    std::int16_t first = 0, end = 0;
};
// World corners and per-view workspaces are allocated once, never inside a frame. Source bytes
// outlive this cache. Only the job holding token() may use its projection workspace.
class TerrainScene {
  public:
    static std::size_t required_bytes(const Elevation& grid);
    static std::optional<TerrainScene> create(const Elevation& grid);
    TerrainScene(TerrainScene&&) noexcept = default;
    TerrainScene& operator=(TerrainScene&&) noexcept = default;
    const Elevation& grid() const { return grid_; }
    std::size_t corner_count() const {
        return static_cast<std::size_t>(grid_.width() + 1) * (grid_.height() + 1);
    }
    std::size_t cell_count() const {
        return static_cast<std::size_t>(grid_.width()) * grid_.height();
    }
    std::size_t bytes() const {
        return corner_count() * 2 * sizeof(Vec3) + cell_count() * sizeof(CellBands) +
               sizeof(colors_) + corner_count() + cell_count();
    }
    Rgb565 color(std::uint8_t index, TerrainStyle style = TerrainStyle::Hypsometric) const {
        return colors_[style == TerrainStyle::CelShaded ? 1 : 0][index];
    }
    void draw_base(Framebuffer& fb, const Camera& camera) const;
    std::uint64_t claim() { return ++token_; }
    std::uint64_t token() const { return token_; }
    void project_corner(std::size_t index, const Camera& camera);
    void bound_cell(std::size_t index, int height, int band_rows);
    void assign_pass(std::size_t index, const PassPlan& plan);
    std::uint8_t cell_pass(std::size_t index) const { return passes_[index]; }
    CellBands cell_bands(std::size_t index) const { return bands_[index]; }
    void draw_cell(Framebuffer& fb, std::size_t index, int band, int band_rows,
                   TerrainStyle style = TerrainStyle::Hypsometric, bool smooth = true) const;

  private:
    TerrainScene(const Elevation& grid, std::unique_ptr<Vec3[]> world,
                 std::unique_ptr<Vec3[]> projected, std::unique_ptr<CellBands[]> bands)
        : grid_(grid), world_(std::move(world)), projected_(std::move(projected)),
          bands_(std::move(bands)) {}
    Elevation grid_;
    std::unique_ptr<Vec3[]> world_, projected_;
    std::unique_ptr<CellBands[]> bands_;
    std::array<std::array<Rgb565, 256>, 2> colors_{};
    std::unique_ptr<std::uint8_t[]> values_;
    std::unique_ptr<std::uint8_t[]> passes_;
    unsigned first_land_ = 256;
    std::uint64_t token_ = 0;
};
constexpr Rgb565 kMovingLand = rgb565(160, 160, 160);
Rgb565 terrain_color(std::int16_t meters, double latitude, TerrainStyle style);
Rgb565 hypsometric_color(std::int16_t meters);
} // namespace globe
