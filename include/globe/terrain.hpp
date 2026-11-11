// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include "globe/elevation.hpp"
#include "globe/framebuffer.hpp"
#include "globe/landclasses.hpp"
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
// Grids with at most this many cells (the 1 degree grid is 64,800) also keep per-pass cell lists,
// 4 bytes per cell. Finer grids do not: the extra megabyte each would break the aggregate budget
// the elevation packs rely on, so they keep scanning every cell on every pass.
constexpr std::size_t kPassListCellLimit = 100000;
struct CellBands {
    std::int16_t first = 0, end = 0;
};
// World corners and per-view workspaces are allocated once, never inside a frame. Source bytes
// outlive this cache. Only the job holding token() may use its projection workspace.
class TerrainScene {
  public:
    static std::size_t required_bytes(const Elevation& grid);
    static constexpr std::size_t list_bytes(std::size_t cells) {
        return cells <= kPassListCellLimit ? cells * sizeof(std::uint32_t) : 0;
    }
    static std::optional<TerrainScene> create(const Elevation& grid,
                                              const LandClasses* classes = nullptr);
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
               sizeof(colors_) + sizeof(class_colors_) + corner_count() + 2 * cell_count() +
               list_bytes(cell_count());
    }
    Rgb565 color(std::uint8_t index, TerrainStyle style = TerrainStyle::Relief,
                 LandClass kind = LandClass::Unknown) const {
        if (style == TerrainStyle::Map)
            return class_colors_[static_cast<unsigned>(kind)][index];
        return colors_[index];
    }
    void draw_base(Framebuffer& fb, const Camera& camera) const;
    std::uint64_t claim() { return ++token_; }
    std::uint64_t token() const { return token_; }
    void project_corner(std::size_t index, const Camera& camera);
    void bound_cell(std::size_t index, int height, int band_rows);
    void assign_pass(std::size_t index, const PassPlan& plan);
    // The ring a cell belongs to (0..kRefinePasses-1), or 255 if it is off the visible disc.
    std::uint8_t cell_pass(std::size_t index) const {
        const auto v = passes_[index];
        return v == kNoPass ? kNoPass : static_cast<std::uint8_t>(v & kPassMask);
    }
    // True if any corner of the cell is behind the horizon (its draw is clipped at the limb).
    bool limb_cell(std::size_t index) const;
    // Per-pass cell lists (only if has_lists()), built by a job right after projecting and
    // bounding: begin_lists() once, then assign_pass() for every cell (it counts), finish_lists()
    // once, then place_cell() for every cell in index order. Pass k's list holds the cells of ring
    // k in index order, so a job walks only the cells it draws instead of testing every cell once
    // per pass.
    bool has_lists() const { return order_ != nullptr; }
    void begin_lists();
    void finish_lists();
    void place_cell(std::size_t index);
    std::size_t pass_count(unsigned pass) const { return start_[pass + 1] - start_[pass]; }
    std::size_t pass_cell(unsigned pass, std::size_t position) const {
        return order_[start_[pass] + position];
    }
    // True if the cell is visible and clipped at the horizon (the limb pass redraws these in index
    // order).
    bool is_limb(std::size_t index) const {
        return passes_[index] != kNoPass && (passes_[index] & kLimbBit) != 0;
    }
    // Advances from `from` over at most `budget` cells that are not limb cells; returns the first
    // limb cell reached, or where it stopped (cell_count() at the end), so one call stays bounded.
    std::size_t skip_to_limb(std::size_t from, std::size_t budget) const;
    CellBands cell_bands(std::size_t index) const { return bands_[index]; }
    void draw_cell(Framebuffer& fb, std::size_t index, int band, int band_rows,
                   TerrainStyle style = TerrainStyle::Relief, bool smooth = true) const;

  private:
    TerrainScene(const Elevation& grid, std::unique_ptr<Vec3[]> world,
                 std::unique_ptr<Vec3[]> projected, std::unique_ptr<CellBands[]> bands)
        : grid_(grid), world_(std::move(world)), projected_(std::move(projected)),
          bands_(std::move(bands)) {}
    Elevation grid_;
    std::unique_ptr<Vec3[]> world_, projected_;
    std::unique_ptr<CellBands[]> bands_;
    std::array<Rgb565, 256> colors_{};
    std::unique_ptr<std::uint8_t[]> values_;
    // Ring in the low four bits, kLimbBit if the cell is clipped at the horizon, or kNoPass.
    static constexpr std::uint8_t kNoPass = 255, kPassMask = 0x0F, kLimbBit = 0x20;
    std::unique_ptr<std::uint8_t[]> passes_;
    std::unique_ptr<std::uint32_t[]> order_; // cells grouped by ring, each group in index order
    std::array<std::uint32_t, kRefinePasses + 1> start_{}; // counts, then offsets into order_
    std::array<std::uint32_t, kRefinePasses> fill_{};
    std::array<std::array<Rgb565, 256>, kLandClassCount> class_colors_{};
    std::unique_ptr<std::uint8_t[]> class_cells_;
    unsigned first_land_ = 256;
    std::uint64_t token_ = 0;
};
constexpr Rgb565 kMovingLand = rgb565(160, 160, 160);
// Map ocean: water cells and the ocean disc share it so the sea has no seams.
constexpr Rgb565 kMapOcean = rgb565(86, 150, 205);
Rgb565 terrain_color(std::int16_t meters, double latitude, TerrainStyle style);
Rgb565 landclass_color(std::int16_t meters, LandClass kind);
Rgb565 hypsometric_color(std::int16_t meters);
} // namespace globe
