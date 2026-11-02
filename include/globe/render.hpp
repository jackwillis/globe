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

namespace globe {

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
    std::uint32_t segments = 0;         // atlas and graticule segments considered
    std::uint32_t segments_visible = 0; // of those, with any part on the front hemisphere
    std::uint32_t lines_drawn = 0;      // straight line pieces rasterized after subdivision
};

constexpr Palette kDefaultPalette = {
    rgb565(8, 12, 24),     // background
    rgb565(24, 64, 120),   // ocean
    rgb565(40, 90, 150),   // graticule
    rgb565(230, 220, 170), // coastline
    rgb565(200, 90, 80),   // border
};

// Geometry that does not depend on the camera, computed once: the unit vector of every atlas
// vertex and of the graticule's sample points. A frame then needs one matrix-vector product per
// vertex instead of four trigonometric calls, which matters on CPUs where libm is slow (a 467
// MHz G4 spends over 100 ns per sin/cos). Costs 24 bytes per atlas vertex plus about 32 KB.
//
// Holds the Atlas view, so the atlas bytes must outlive the Scene.
class Scene {
  public:
    // One allocation, made up front; nullopt if it fails.
    static std::optional<Scene> create(const Atlas& atlas);

    Scene(Scene&&) noexcept = default;
    Scene& operator=(Scene&&) noexcept = default;
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    ~Scene() = default;

    const Atlas& atlas() const { return atlas_; }
    const Vec3* vertices() const { return storage_.get(); } // atlas point order
    const Vec3* graticule() const { return storage_.get() + atlas_.point_count(); }
    std::size_t bytes() const;

  private:
    Scene(const Atlas& atlas, std::unique_ptr<Vec3[]> storage)
        : atlas_(atlas), storage_(std::move(storage)) {}

    Atlas atlas_;
    std::unique_ptr<Vec3[]> storage_; // atlas vertices, then graticule samples
};

// Clears the framebuffer to the background and draws one frame, using the precomputed vectors.
RenderStats render_globe(Framebuffer& fb, const Camera& camera, const Scene& scene,
                         const Palette& palette, const Layers& layers);

// Reference path: same pixels, but computes every vertex's unit vector on the fly. Used by tests
// to check the Scene path, and by the benchmark to show what the cache saves.
RenderStats render_globe(Framebuffer& fb, const Camera& camera, const Atlas& atlas,
                         const Palette& palette, const Layers& layers);

} // namespace globe
