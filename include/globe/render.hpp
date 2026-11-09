// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Draws the globe: ocean disc, graticule, coastlines, borders (design doc section 6 layer order,
// minus the layers that do not exist yet). Pure function of (camera, atlas, palette, layers):
// the same inputs give the same pixels within one build.
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include "globe/atlas.hpp"
#include "globe/framebuffer.hpp"
#include "globe/projection.hpp"
#include "globe/refine_order.hpp"
#include "globe/terrain_style.hpp"

namespace globe {
class TerrainScene;

struct RenderQuality {
    double max_chord_px = 3.0;
    bool terrain = false;
    unsigned elevation_level = 0;
    TerrainStyle terrain_style = TerrainStyle::Relief;
    bool smooth_terrain = true;
    // Draw the coarse gray land mask under the lines. Off: while moving, the globe is plain vectors
    // (owner's choice, 2026-11-02); the mask stays available for a later style.
    bool gray_base = false;
    ScreenPoint focus{};
    bool preserve_bitmap = false; // subsequent resolution replaces the existing terrain
    bool terrain_only = false;    // lines belong to the final resolution only
    // Map look, smooth terrain, a job that clears the disc itself (not preserve_bitmap): leave out
    // the ocean cells that would only repaint the cleared ocean. The settled pixels are identical
    // (tests/test_terrain.cpp compares random views); off only so tests can prove that.
    bool skip_ocean = true;
    // Terrain jobs only: draw the lines first into a private layer and composite them over the
    // frame after every terrain pass, so they are visible from the first presented frame. The
    // settled pixels are identical. Replaces the paced line reveal.
    bool lines_first = false;
    // Entries in the line-pixel list the composites use (0: a sixteenth of the frame). A view with
    // more line pixels falls back to scanning the layer; tests set it small to exercise that.
    std::size_t line_list_limit = 0;
    // render_globe only: draw the layers over what the frame already holds (no clear, no ocean),
    // the way a refinement job does, so lines can go on top of a scaled zoom preview.
    bool lines_only = false;
};

struct Palette {
    Rgb565 background;
    Rgb565 ocean;
    Rgb565 graticule;
    Rgb565 coastline;
    Rgb565 border;
};

struct Layers {
    bool graticule = true;
    bool coastlines = true;
    bool borders = true;
};

// What a frame cost, in countable terms (not time; timing belongs to the platform layer).
struct RenderStats {
    std::uint32_t terrain_slices = 0;    // admitted cell/16-row slices, including the final repaint
    std::uint32_t polylines_culled = 0;  // whole cached polylines proven behind the horizon
    std::uint32_t segments = 0;          // atlas and graticule segments considered
    std::uint32_t segments_visible = 0;  // of those, with any part on the front hemisphere
    std::uint32_t lines_drawn = 0;       // straight line pieces rasterized after subdivision
    std::uint32_t polylines_skipped = 0; // refinement only: polylines proven outside the band
    std::uint32_t segments_skipped = 0;  // refinement only: arcs proven outside the band
};

Palette palette_for_style(Palette palette, TerrainStyle style);

constexpr Palette kDefaultPalette = {
    rgb565(8, 12, 24),     // background
    rgb565(24, 64, 120),   // ocean
    rgb565(40, 90, 150),   // graticule
    rgb565(230, 220, 170), // coastline
    rgb565(200, 90, 80),   // border
};

// Conservative spherical cap. sin_radius == 1 disables culling for a cap >= 90 degrees or
// a degenerate mean direction. Smaller caps contain every vertex and their shorter arcs.
struct PolylineCap {
    Vec3 axis;
    double sin_radius;
};

// Geometry that does not depend on the camera, computed once: the unit vector of every atlas
// vertex and of the graticule's sample points. A frame then needs one matrix-vector product per
// vertex instead of four trigonometric calls, which matters on CPUs where libm is slow (a 467
// MHz G4 spends over 100 ns per sin/cos). Costs 24 bytes per atlas vertex, about 32 KB for
// the graticule, and sizeof(PolylineCap) per atlas polyline.
//
// Holds the Atlas view, so the atlas bytes must outlive the Scene.
class Scene {
  public:
    // Up-front geometry and caps; nullopt on failure.
    static std::optional<Scene> create(const Atlas& atlas);

    Scene(Scene&&) noexcept = default;
    Scene& operator=(Scene&&) noexcept = default;
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    ~Scene() = default;

    const Atlas& atlas() const { return atlas_; }
    const Vec3* vertices() const { return storage_.get(); } // atlas point order
    const Vec3* graticule() const { return storage_.get() + atlas_.point_count(); }
    const PolylineCap* caps() const { return caps_.get(); } // atlas polyline order
    std::size_t bytes() const;

  private:
    Scene(const Atlas& atlas, std::unique_ptr<Vec3[]> storage, std::unique_ptr<PolylineCap[]> caps)
        : atlas_(atlas), storage_(std::move(storage)), caps_(std::move(caps)) {}

    Atlas atlas_;
    std::unique_ptr<Vec3[]> storage_; // atlas vertices, then graticule samples
    std::unique_ptr<PolylineCap[]> caps_;
};

// Clears the framebuffer and draws using cached vectors. Disable culling for reference checks.
RenderStats render_globe(Framebuffer& fb, const Camera& camera, const Scene& scene,
                         const Palette& palette, const Layers& layers, bool cull_polylines = true,
                         RenderQuality quality = {}, const TerrainScene* base = nullptr);

// Reference path: same pixels, but computes every vertex's unit vector on the fly. Used by tests
// to check the Scene path, and by the benchmark to show what the cache saves.
RenderStats render_globe(Framebuffer& fb, const Camera& camera, const Atlas& atlas,
                         const Palette& palette, const Layers& layers,
                         const TerrainScene* base = nullptr);

constexpr int kRefineBandRows = 16;
struct RenderStep {
    unsigned work_units = 0;
    int first_row = 0;
    int end_row = 0;
    bool band_ready = false; // the platform may present rows [first_row, end_row)
};

// One bounded continuation, no allocations. Scene must outlive the job. A work unit is one
// background row, graticule/atlas arc, or polyline/table transition. Every step restores fb's clip.
class RefinementJob {
  public:
    RefinementJob(const Camera& camera, const Scene& scene, const Palette& palette, Layers layers,
                  RenderQuality quality, std::uint64_t generation, TerrainScene* terrain = nullptr);
    bool active() const { return active_; }
    // Terrain jobs with lines_first: copy the finished terrain (before the lines go on) into
    // `snapshot` when the last stamp starts. snapshot_taken() then stays true.
    void set_snapshot(Framebuffer* snapshot) { snapshot_ = snapshot; }
    bool snapshot_taken() const { return snapshot_taken_; }
    // How far along this job is, 0..1: monotonic, rough (set-up, line layer, the ring passes, the
    // limb pass), meant for a progress bar.
    double progress() const;
    void cancel() { active_ = false; }
    RenderStep step(Framebuffer& fb, std::uint64_t generation, unsigned max_work_units = 64);
    const RenderStats& stats() const { return stats_; }

  private:
    TerrainScene* terrain_ = nullptr;
    std::uint64_t terrain_token_ = 0;
    std::size_t prepare_index_ = 0;
    std::size_t terrain_cell_ = 0; // the limb pass: next cell to look at
    std::size_t list_pos_ = 0;     // the ring passes: position in the pass's cell list
    Camera camera_;
    const Scene* scene_;
    Palette palette_;
    Layers layers_;
    RenderQuality quality_;
    std::uint64_t generation_;
    bool active_ = true;
    PassPlan plan_{};
    unsigned pass_ = 0;
    int cell_band_ = -1;
    int band_first_ = 0;
    int row_ = 0;
    int phase_ = 0;
    int grid_segment_ = 0;
    std::optional<Framebuffer> line_layer_; // lines_first: key-colored except where a line is
    // Where the line layer has a line: (row << 16 | column), so each composite touches only those
    // pixels (a few percent of the frame) instead of scanning the whole layer. If a view has more
    // than the list holds, the composite falls back to the scan.
    std::unique_ptr<std::uint32_t[]> line_list_;
    std::size_t line_list_count_ = 0;
    std::size_t line_list_capacity_ = 0;
    bool line_list_ok_ = false;
    std::size_t stamp_pos_ = 0; // entries (or rows, in the fallback) already composited this stamp
    Framebuffer* snapshot_ = nullptr;
    bool snapshot_taken_ = false;
    bool lines_drawing_ = false; // phases 1-4 currently draw into line_layer_
    int after_stamp_ = 7;
    AtlasCursor cursor_{};
    AtlasPolyline line_{AtlasLayer::Coastline, 0, nullptr};
    std::uint16_t segment_ = 0;
    std::uint32_t line_first_ = 0;
    RenderStats stats_{};
};

} // namespace globe
