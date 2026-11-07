// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/zoom.hpp"
#include <cmath>
#include <cstring>
namespace globe {
bool same_orientation(const Camera& a, const Camera& b) {
    const auto close = [](const Vec3& u, const Vec3& v) {
        return std::fabs(u.x - v.x) < 1e-9 && std::fabs(u.y - v.y) < 1e-9 &&
               std::fabs(u.z - v.z) < 1e-9;
    };
    return a.width == b.width && a.height == b.height && a.center_x == b.center_x &&
           a.center_y == b.center_y && close(a.right, b.right) && close(a.up, b.up) &&
           close(a.forward, b.forward);
}
Framebuffer* ZoomPreview::begin_job(const Camera& camera, TerrainStyle style, int width,
                                    int height) {
    if (!cache_.has_value() || cache_->width() != width || cache_->height() != height) {
        valid_ = false;
        cache_ = Framebuffer::create(width, height);
    }
    if (!cache_.has_value())
        return nullptr;
    pending_ = camera;
    pending_style_ = style;
    pending_set_ = true;
    return &*cache_;
}
void ZoomPreview::after_step(const RefinementJob& job) {
    if (!job.snapshot_taken() || !cache_.has_value() || !pending_set_)
        return;
    // The buffer now holds this job's terrain (replacing a partial frame kept earlier, if any).
    valid_ = true;
    baked_ = false;
    camera_ = pending_;
    style_ = pending_style_;
}
void ZoomPreview::keep_partial(const Framebuffer& fb, bool job_active, const Camera& now) {
    if (!job_active || !pending_set_)
        return;
    const bool useful = valid_ && cache_.has_value() && same_orientation(camera_, pending_) &&
                        style_ == pending_style_;
    if (useful || !same_orientation(pending_, now))
        return; // keep the older cache, or this is not a view we could zoom from
    if (!cache_.has_value() || cache_->width() != fb.width() || cache_->height() != fb.height())
        cache_ = Framebuffer::create(fb.width(), fb.height());
    if (!cache_.has_value())
        return;
    std::memcpy(cache_->data(), fb.data(), fb.byte_size());
    valid_ = true;
    baked_ = true;
    camera_ = pending_;
    style_ = pending_style_;
}
bool ZoomPreview::preview(Framebuffer& fb, const Camera& camera, TerrainStyle style,
                          Rgb565 background, Rgb565 ocean) {
    if (!valid_ || !cache_.has_value() || style != style_ || !same_orientation(camera, camera_) ||
        cache_->width() != fb.width() || cache_->height() != fb.height())
        return false;
    const double factor = camera.radius_px / camera_.radius_px;
    if (!(factor >= kZoomPreviewMin && factor <= kZoomPreviewMax))
        return false;
    return scale_frame(*cache_, fb, camera.center_x, camera.center_y, factor, background, ocean,
                       camera.radius_px, columns_) != kInvalidZoom;
}
std::size_t scale_frame(const Framebuffer& src, Framebuffer& dst, double cx, double cy,
                        double factor, Rgb565 outside, Rgb565 hole, double disc_radius,
                        std::vector<int>& columns) {
    if (src.width() != dst.width() || src.height() != dst.height() || !std::isfinite(factor) ||
        factor <= 0 || !std::isfinite(cx) || !std::isfinite(cy))
        return kInvalidZoom;
    const int w = dst.width(), h = dst.height();
    columns.resize(static_cast<std::size_t>(w));
    for (int x = 0; x < w; ++x) {
        const int sx = static_cast<int>(std::floor(cx + (x + 0.5 - cx) / factor));
        columns[static_cast<std::size_t>(x)] = sx >= 0 && sx < w ? sx : -1;
    }
    std::size_t holes = 0;
    const double limit = disc_radius * disc_radius;
    for (int y = 0; y < h; ++y) {
        const int sy = static_cast<int>(std::floor(cy + (y + 0.5 - cy) / factor));
        Rgb565* out =
            dst.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(dst.stride());
        const bool row_ok = sy >= 0 && sy < h;
        const Rgb565* in = row_ok ? src.data() + static_cast<std::size_t>(sy) *
                                                     static_cast<std::size_t>(src.stride())
                                  : nullptr;
        const double dy = y + 0.5 - cy;
        for (int x = 0; x < w; ++x) {
            const int sx = columns[static_cast<std::size_t>(x)];
            if (row_ok && sx >= 0) {
                out[x] = in[sx];
                continue;
            }
            const double dx = x + 0.5 - cx;
            if (dx * dx + dy * dy <= limit) {
                out[x] = hole;
                ++holes;
            } else {
                out[x] = outside;
            }
        }
    }
    return holes;
}
} // namespace globe
