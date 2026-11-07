// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// What both front ends do around a RefinementJob, in one place: start it for the model (over a
// zoom preview if one is on screen), cancel it when input arrives (keeping the half-refined frame
// as a preview source if nothing better exists), step it, draw the city markers after each finished
// band, and render the moving frame (a zoom preview or plain vectors). The front ends keep the
// event loop, the wake timers and getting pixels onto the screen.
#pragma once
#include "globe/app.hpp"
#include "globe/framebuffer.hpp"
#include "globe/places.hpp"
#include "globe/render.hpp"
#include "globe/zoom.hpp"
#include <optional>
namespace globe {
class RefineRunner {
  public:
    // The scene must outlive the runner; bind it before the first use (or construct with it).
    RefineRunner() = default;
    explicit RefineRunner(const Scene& scene) : scene_(&scene) {}
    void set_scene(const Scene& scene) { scene_ = &scene; }
    // City markers drawn after each finished band; null (the default) draws none.
    void set_places(const PlaceScene* places) { places_ = places; }
    void enable_preview(bool on) { preview_enabled_ = on; }

    // The model asked for a refinement (Effects::refine): build its job. `fb` is the frame it will
    // paint into; `terrain` the elevation pack for the model's target level.
    void start(const Model& model, Framebuffer* fb, TerrainScene* terrain);
    // Input changed the model's generation: stop the job. `fb` holds the frame shown so far and
    // `now` is the model's camera after the input.
    void cancel(const Framebuffer* fb, const Camera& now);
    bool active() const { return job_.has_value() && job_->active(); }
    const RefinementJob* job() const { return job_.has_value() ? &*job_ : nullptr; }

    // One bounded continuation. City markers are drawn after a band is ready, before the caller
    // shows it. Call only while active().
    RenderStep step(Framebuffer& fb, const Model& model, unsigned max_work_units = 64);

    // Renders the frame for a model that is moving. Returns true if a zoom preview was used.
    bool render_moving(Framebuffer& fb, const Model& model, TerrainScene* terrain);

    const ZoomPreview& zoom() const { return zoom_; }

  private:
    const Scene* scene_ = nullptr;
    const PlaceScene* places_ = nullptr;
    std::optional<RefinementJob> job_;
    ZoomPreview zoom_;
    bool preview_enabled_ = true;
    bool preview_shown_ = false; // fb holds a zoom preview, not a cleared frame
};
} // namespace globe
