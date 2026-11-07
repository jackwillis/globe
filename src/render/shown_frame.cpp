// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/shown_frame.hpp"
#include <cstring>
namespace globe {
bool frame_settled(const Model& model, bool job_active) {
    return model.stage == QualityStage::Refine &&
           (!model.refine_active || model.refine_level == model.refine_target) && !job_active;
}
double bar_progress(const Model& model, const RefinementJob* job) {
    if (!model.refine_active)
        return -1.0;
    const double now = refine_progress(model, job != nullptr ? job->progress() : 0.0);
    return now < 1.0 ? now : -1.0;
}
const Framebuffer& ShownFrame::begin(Overlays& overlays, Framebuffer& fb, const Camera& camera,
                                     std::int64_t unix_seconds, bool settled, double progress) {
    Framebuffer* shown = overlays.compose(fb, camera, unix_seconds, !settled);
    quick_ = shown != nullptr && !settled;
    bar_ = progress >= 0.0 && fb.height() >= kProgressBarRows;
    rows_ = nullptr;
    if (bar_) {
        if (shown == nullptr) {
            const std::size_t count = static_cast<std::size_t>(fb.width()) * kProgressBarRows;
            rows_ = fb.data() + static_cast<std::size_t>(fb.height() - kProgressBarRows) *
                                    static_cast<std::size_t>(fb.stride());
            stash_.resize(count);
            std::memcpy(stash_.data(), rows_, count * sizeof(Rgb565));
            shown = &fb;
        }
        draw_progress_bar(*shown, progress);
    }
    return shown != nullptr ? *shown : fb;
}
void ShownFrame::end() {
    if (rows_ != nullptr)
        std::memcpy(rows_, stash_.data(), stash_.size() * sizeof(Rgb565));
    rows_ = nullptr;
}
} // namespace globe
