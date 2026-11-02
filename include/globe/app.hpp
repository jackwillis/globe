// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Application state in the Elm style: a Model, the Msgs that can change it, and an update()
// that is a pure function of (Model, Msg). Time arrives inside messages and side effects are
// returned as data (Effects) for the platform layer to run, so any recorded sequence of
// messages replays to identical states.
//
// Holds no framebuffer, atlas or geometry: the view layer renders from a read-only Model.
#pragma once

#include <cstdint>
#include <variant>

#include "globe/projection.hpp"
#include "globe/render.hpp"

namespace globe {

// Rendering quality plan (design doc section 4). M1 only ever uses Moving; the rest are named so
// the Model does not change shape when staged refinement arrives in M2.
enum class QualityStage : std::uint8_t { Moving, Settle, Refine, Decorate, Fetch };

enum class Key : std::uint8_t {
    Left,
    Right,
    Up,
    Down,
    ZoomIn,
    ZoomOut,
    Reset,
    ToggleGraticule,
    ToggleBorders,
    ToggleCoastlines,
    Quit,
};

struct Model {
    Camera camera;
    Layers layers;
    QualityStage stage;
    bool dragging;
    ScreenPoint drag_last;
    std::uint64_t last_input_ms; // time of the most recent user input, from the message
};

// Messages. Coordinates are viewport pixels; time_ms is a monotonic clock supplied by the
// platform (or a trace), never read inside update().
struct PointerDown {
    std::uint64_t time_ms;
    double x;
    double y;
};
struct PointerMove {
    std::uint64_t time_ms;
    double x;
    double y;
};
struct PointerUp {
    std::uint64_t time_ms;
};
struct Wheel {
    std::uint64_t time_ms;
    int steps; // positive zooms in
};
struct KeyDown {
    std::uint64_t time_ms;
    Key key;
};
struct Resize {
    int width;
    int height;
};
struct Repaint {};

using Msg = std::variant<PointerDown, PointerMove, PointerUp, Wheel, KeyDown, Resize, Repaint>;

struct Effects {
    bool redraw = false;
    bool quit = false;
};

// Fresh state for a viewport: north up, centered on a fixed home position, globe sized to fit.
// Dimensions outside 1..kMaxViewportDimension are clamped.
Model initial_model(int width, int height);

// Smallest and largest allowed globe radius for a viewport.
double min_radius_px(int width, int height);
double max_radius_px(int width, int height);

Effects update(Model& model, const Msg& msg);

} // namespace globe
