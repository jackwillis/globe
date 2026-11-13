// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Everything drawn over a finished frame, shared by the front ends: the night side, a measured
// great-circle arc and the focused country. The rendered frame is never changed; `compose` returns
// a copy with the overlays on it, so clearing one needs no re-render and no pin moves.
#pragma once
#include "globe/countries.hpp"
#include "globe/framebuffer.hpp"
#include "globe/places.hpp"
#include "globe/projection.hpp"
#include <cstdint>
#include <optional>
namespace globe {
constexpr int kProgressBarRows = 10;

constexpr Rgb565 kProgressFill = rgb565(255, 255, 255);
constexpr Rgb565 kProgressEdge = rgb565(0, 0, 0);
// Opaque, nothing blended: in the bottom kProgressBarRows rows of `fb`, columns [0, progress *
// width) get a one-pixel black edge on top and white below it; the rest of the map is left as it
// is. The caller draws it on a frame it will put back (or a copy), never on the finished frame.
void draw_progress_bar(Framebuffer& fb, double progress);

class Overlays {
  public:
    // The pick data are optional (a build may omit them); the pointers must outlive this object.
    void attach(const Countries* countries, const CountryScene* country_scene, const Places* places,
                const PlaceScene* place_scene) {
        countries_ = countries;
        country_scene_ = country_scene;
        places_ = places;
        place_scene_ = place_scene;
    }
    bool night() const { return night_; }
    bool measuring() const { return measuring_; }
    std::optional<std::uint32_t> focused() const { return focused_; }
    // True when something is drawn over the frame.
    bool active() const { return night_ || focused_.has_value() || measure_a_.has_value(); }
    void toggle_night() { night_ = !night_; }
    // Units for card text (the shell takes them from Model). A finished measurement's distance card
    // is remade in the new units and offered again through `take_card`.
    void set_preferences(const Preferences& prefs);
    // Leaving measure mode clears the measured points.
    void toggle_measure();
    // A click that survived the double-click wait, at screen point `at`. Handles measuring, places
    // and countries; returns true when what is drawn over the frame changed. A card to show, if
    // any, is taken with `take_card` (its text stays valid until the next click).
    bool click(const Camera& camera, ScreenPoint at);
    // Whether a click at `at` would do something beyond starting a drag: while measuring, anywhere
    // on the globe; otherwise on a place. Countries are not counted (an owner decision still
    // open), and no country test runs here, so this is cheap enough for every pointer motion.
    bool clickable_at(const Camera& camera, ScreenPoint at) const;
    std::optional<InfoCard> take_card();
    // The frame with the active overlays drawn on a copy kept here, or null when none is active.
    // `unix_seconds` is UTC; the sun position is refreshed at most every 30 s. With `quick` (frames
    // that move, or are still refining) the night side is only a terminator line and the country
    // is outlined without the dither fill, so dragging stays fast; the full overlays come with the
    // settled frame.
    Framebuffer* compose(const Framebuffer& frame, const Camera& camera, std::int64_t unix_seconds,
                         bool quick = false);

  private:
    void measure_click(LatLon point);
    void make_measure_card(LatLon a, LatLon p);
    const Countries* countries_ = nullptr;
    const CountryScene* country_scene_ = nullptr;
    const Places* places_ = nullptr;
    const PlaceScene* place_scene_ = nullptr;
    Preferences prefs_{};
    bool night_ = false, measuring_ = false;
    std::optional<std::uint32_t> focused_;
    std::optional<LatLon> measure_a_, measure_b_;
    std::optional<Framebuffer> copy_;
    Vec3 sun_{1, 0, 0};
    std::int64_t sun_time_ = 0;
    char text_[4][64] = {};
    InfoCard card_{};
    bool card_ready_ = false;
};
} // namespace globe
