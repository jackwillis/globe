// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Cheap zoom preview. An orthographic globe zoomed about its center is the same picture scaled
// about that point, so a finished frame can stand in for the real render while the wheel turns.
#pragma once
#include "globe/framebuffer.hpp"
#include "globe/projection.hpp"
#include "globe/render.hpp"
#include "globe/terrain_style.hpp"
#include <optional>
#include <vector>
namespace globe {
// Nearest-neighbor copy of `src` scaled by `factor` about (cx, cy) into `dst` (same size). A pixel
// whose source lies outside `src` gets `outside` if it is beyond `disc_radius` of the center and
// `hole` if it is inside the disc (the preview has nothing to show there). Returns the number of
// hole pixels, or kInvalidZoom for a size mismatch or a non-finite or non-positive argument.
// `columns` is caller-owned scratch (resized once per size, so steady state does not allocate).
constexpr std::size_t kInvalidZoom = static_cast<std::size_t>(-1);
std::size_t scale_frame(const Framebuffer& src, Framebuffer& dst, double cx, double cy,
                        double factor, Rgb565 outside, Rgb565 hole, double disc_radius,
                        std::vector<int>& columns);

// The cached frame stands in for a render across this range of scale factors; beyond it the plain
// vector frame is drawn (a 4x blow-up is a few big pixels, but it is still the right picture).
constexpr double kZoomPreviewMin = 0.25;
constexpr double kZoomPreviewMax = 4.0;

bool same_orientation(const Camera& a, const Camera& b);

// The platform-neutral half of the zoom preview: which frame to scale from, and when it is usable.
// A platform asks begin_job() for the buffer a refinement job snapshots its finished terrain into
// (set_snapshot), tells the cache after every step, keeps the half-refined frame when input cancels
// a job, and calls preview() instead of rendering while the view only zooms about its center.
class ZoomPreview {
  public:
    // A terrain job is starting for `camera`; returns its snapshot buffer (null if out of memory).
    // The previous cached frame stays valid until the new snapshot replaces it.
    Framebuffer* begin_job(const Camera& camera, TerrainStyle style, int width, int height);
    // After each job step: adopt the snapshot once the job has taken it.
    void after_step(const RefinementJob& job);
    // Input cancelled a job part-way. If the cache is no use for the job's view (right after a
    // drag), the half-refined `fb` is the best picture of it to zoom from; its lines are baked in.
    void keep_partial(const Framebuffer& fb, bool job_active, const Camera& now);
    // Scales the cache into `fb` for `camera`. False (fb untouched or garbage) when there is no
    // usable cache. Ocean fills holes at the rim. The caller draws fresh lines unless baked().
    bool preview(Framebuffer& fb, const Camera& camera, TerrainStyle style, Rgb565 background,
                 Rgb565 ocean);
    bool baked() const { return baked_; }
    void invalidate() { valid_ = false; }
    bool valid() const { return valid_; }
    // Frame buffers or terrain changed: the cache is for something else.
    void reset() {
        valid_ = false;
        cache_.reset();
    }

  private:
    std::optional<Framebuffer> cache_; // terrain only, or with its lines if baked_
    bool valid_ = false;
    bool baked_ = false;
    Camera camera_{};
    Camera pending_{};
    TerrainStyle style_ = TerrainStyle::Map;
    TerrainStyle pending_style_ = TerrainStyle::Map;
    bool pending_set_ = false;
    std::vector<int> columns_;
};
} // namespace globe
