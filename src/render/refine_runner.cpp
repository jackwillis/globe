// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/refine_runner.hpp"
namespace globe {
void RefineRunner::start(const Model& model, Framebuffer* fb, TerrainScene* terrain) {
    auto quality = quality_for(model);
    // A zoom preview is on screen: refine over it instead of clearing to the ocean.
    quality.preserve_bitmap = quality.preserve_bitmap || preview_shown_;
    preview_shown_ = false;
    job_.emplace(model.camera, *scene_, kDefaultPalette, effective_layers(model), quality,
                 model.generation, terrain);
    if (quality.terrain && fb != nullptr && preview_enabled_) { // every level: coarse previews too
        if (auto* snapshot =
                zoom_.begin_job(model.camera, model.terrain_style, fb->width(), fb->height()))
            job_->set_snapshot(snapshot);
    }
}
void RefineRunner::cancel(const Framebuffer* fb, const Camera& now) {
    if (fb != nullptr && preview_enabled_)
        zoom_.keep_partial(*fb, active(), now);
    if (job_.has_value())
        job_->cancel();
}
RenderStep RefineRunner::step(Framebuffer& fb, const Model& model, unsigned max_work_units) {
    const auto result = job_->step(fb, model.generation, max_work_units);
    zoom_.after_step(*job_);
    if (result.band_ready && places_ != nullptr) {
        fb.clip_rows(result.first_row, result.end_row);
        places_->draw(fb, model.camera);
        fb.clip_rows(0, fb.height());
    }
    return result;
}
bool RefineRunner::render_moving(Framebuffer& fb, const Model& model, TerrainScene* terrain) {
    // While the view only zooms about its center, the last finished terrain frame scaled to the new
    // radius, with fresh lines on top, stands in for a render (unless its lines are baked in).
    if (preview_enabled_ && model.stage == QualityStage::Moving) {
        const auto palette = palette_for_style(kDefaultPalette, model.terrain_style);
        if (zoom_.preview(fb, model.camera, model.terrain_style, palette.background,
                          palette.ocean)) {
            if (!zoom_.baked()) {
                auto quality = quality_for(model);
                quality.lines_only = true;
                render_globe(fb, model.camera, *scene_, kDefaultPalette, effective_layers(model),
                             true, quality, terrain);
            }
            preview_shown_ = true;
            return true;
        }
    }
    preview_shown_ = false;
    render_globe(fb, model.camera, *scene_, kDefaultPalette, effective_layers(model), true,
                 quality_for(model), terrain);
    return false;
}
} // namespace globe
