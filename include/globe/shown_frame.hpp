// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// The frame a front end converts and shows, built from the rendered one without changing it: the
// overlays (night, measure, focused country) on a copy, then the scale at the center, then the
// refinement progress bar. Both
// front ends build it the same way, so what is overlaid and when cannot drift apart.
#pragma once
#include "globe/app.hpp"
#include "globe/framebuffer.hpp"
#include "globe/overlays.hpp"
#include "globe/render.hpp"
#include "globe/scale_overlay.hpp"
#include <cstdint>
#include <vector>
namespace globe {
// Settled frames get the full overlays (night shading, country fill); frames that move or are still
// refining get the cheap ones. `job_active`: a refinement job is still stepping.
bool frame_settled(const Model& model, bool job_active);
// Progress-bar fraction 0..1 while a refinement runs (between two levels too), or -1 for no bar.
// `job` may be null.
double bar_progress(const Model& model, const RefinementJob* job);

class ShownFrame {
  public:
    // The frame to convert for `fb`: an overlay copy if any overlay is active, else `fb` itself.
    // The bar (progress >= 0) goes on the copy, or straight into `fb` with its rows stashed, so
    // call end() once the frame has been converted. Valid until end().
    const Framebuffer& begin(Overlays& overlays, Framebuffer& fb, const Camera& camera,
                             std::int64_t unix_seconds, bool settled, double progress);
    // A label for the scale drawn by the front end with a system font, or null for the built-in
    // digits. Not owned; keep it alive while frames are built.
    void set_scale_label(const LabelBitmap* label) { label_ = label; }
    // Puts the frame back as it was rendered.
    void end();
    // The last begin() drew overlays on a cheap (quick) copy: a settled frame should replace it.
    bool quick() const { return quick_; }
    // The last begin() drew a bar: a frame without one should replace it.
    bool bar() const { return bar_; }

  private:
    std::vector<Rgb565> stash_;
    Rgb565* rows_ = nullptr; // where the stashed rows go back
    bool quick_ = false;
    bool bar_ = false;
    const LabelBitmap* label_ = nullptr;
};
} // namespace globe
