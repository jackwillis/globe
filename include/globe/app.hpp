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

// How dragging turns the globe.
//  Turntable: the center's latitude and longitude change at a constant angle per pixel (scaled
//             by zoom), so north always stays up and every part of the window behaves alike.
//  Trackball: the point under the pointer follows it exactly; the globe can roll, and dragging
//             near the rim turns it faster.
enum class DragMode : std::uint8_t { Turntable, Trackball };

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
    AlignNorth,     // rotate about the view axis so north is up; the center stays put
    ToggleDragMode, // switch between Turntable and Trackball
    Quit,
};

struct Model {
    Camera camera;
    Layers layers;
    DragMode drag_mode;
    QualityStage stage;
    bool dragging;
    bool drag_moved; // the camera changed since the button went down
    ScreenPoint drag_last;
    bool click_valid; // a recent press that could become the first half of a double click
    ScreenPoint click_pos;
    std::uint64_t click_ms;
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

// Layers to draw for the current state. The graticule is hidden while a drag is in progress to
// keep frames cheap; releasing the button redraws it.
Layers effective_layers(const Model& model);

// Two presses within kDoubleClickMs and kDoubleClickSlopPx, the second on the globe, zoom in by
// kDoubleClickZoom and center the globe on the clicked point.
constexpr std::uint64_t kDoubleClickMs = 400;
constexpr double kDoubleClickSlopPx = 6.0;
constexpr double kDoubleClickZoom = 2.0;

Effects update(Model& model, const Msg& msg);

} // namespace globe
