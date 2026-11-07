// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Application state in the Elm style: a Model, the Msgs that can change it, and an update()
// that is a pure function of (Model, Msg). Time arrives inside messages and side effects are
// returned as data (Effects) for the platform layer to run, so any recorded sequence of
// messages replays to identical states.
//
// Holds no framebuffer, atlas or geometry: the view layer renders from a read-only Model.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>

#include "globe/projection.hpp"
#include "globe/render.hpp"

namespace globe {

// Rendering quality plan (design doc section 4). Ticks advance Moving -> Settle -> Refine.
enum class QualityStage : std::uint8_t { Moving, Settle, Refine, Decorate, Fetch };

// How dragging turns the globe.
//  Turntable: the center's latitude and longitude change at a constant angle per pixel (scaled
//             by zoom), so north always stays up and every part of the window behaves alike.
//  Trackball: the point under the pointer follows it exactly; the globe can roll, and dragging
//             near the rim turns it faster.
enum class DragMode : std::uint8_t { Turntable, Trackball };

enum class Command : std::uint8_t {
    Left,
    Right,
    Up,
    Down,
    ZoomIn,
    ZoomOut,
    Reset,
    ToggleGraticule,
    ToggleBorders,
    AlignNorth,     // rotate about the view axis so north is up; the center stays put
    ToggleDragMode, // switch between Turntable and Trackball
    Quit,
    CycleStyle,
};

constexpr unsigned kElevationLevels = 4;
struct ElevationLevel {
    std::uint32_t width = 0, height = 0;
};
struct Model {
    std::array<ElevationLevel, kElevationLevels> elevations{};
    unsigned refine_level = 0, refine_target = 0;
    bool refine_active = false, refine_first = true;
    TerrainStyle terrain_style = TerrainStyle::Map;
    Camera camera;
    Layers layers;
    DragMode drag_mode;
    QualityStage stage;
    std::uint64_t generation; // invalidates in-flight screen-space work on accepted input/resize
    std::uint64_t stage_since_ms; // settle clock; separate from last user input for resize
    bool stage_needs_time;        // initial state or resize awaits its first timestamped message
    bool dragging;
    bool drag_active; // the pointer has moved past kDragSlopPx since the button went down
    bool drag_moved;  // the camera changed since the button went down
    ScreenPoint drag_last;
    bool click_valid; // a recent press that could become the first half of a double click
    ScreenPoint click_pos;
    std::uint64_t click_ms;
    std::uint64_t last_input_ms; // time of the most recent user input, from the message
    bool pick_pending; // a plain click waits kPickDelayMs for a second click before it picks
    ScreenPoint pick_pos;
    std::uint64_t pick_due_ms;
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
struct CommandIssued {
    std::uint64_t time_ms;
    Command command;
};
struct Resize {
    int width;
    int height;
};
struct Repaint {};
struct Tick {
    std::uint64_t time_ms;
};

struct RegisterElevation {
    unsigned level;
    std::uint32_t width, height;
};

struct Refine {
    std::uint64_t generation;
    unsigned level;
};
struct RefinementFinished {
    std::uint64_t generation;
    unsigned level;
};

using Msg = std::variant<PointerDown, PointerMove, PointerUp, Wheel, CommandIssued, Resize, Repaint,
                         Tick, RegisterElevation, RefinementFinished>;

struct Effects {
    std::optional<Refine> refine;
    bool redraw = false;
    bool quit = false;
    // Request a wake (a Tick message) after this delay; 0 means no new request. A new request
    // replaces any earlier one that has not fired: platforms keep a single deadline (an X11 select
    // timeout, the sleep argument of WaitNextEvent), never one timer per request, because input
    // such as pointer motion asks for a wake on every message. A stale wake that does fire is
    // harmless: update checks the current stage and input timestamp.
    std::uint32_t wake_after_ms = 0;
    // Show the pixels already in the framebuffer again (window exposed, update event). Nothing
    // may be rendered or restarted for it: the model did not change. A platform with no valid
    // frame yet (the first expose, or after its buffers were recreated) renders instead.
    bool present_only = false;
    // A plain click was not followed by a second click in time: pick the country or place under
    // `pick_at` (view coordinates). The model is unchanged; nothing is rendered for it.
    bool pick = false;
    ScreenPoint pick_at{};
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
RenderQuality quality_for(const Model& model);
// Deterministic start effect, also used by explicit headless refinement benchmarks.
Effects start_refinement(Model& model);

// One entry per Command, indexed by its enum value. All current commands are visible;
// checked describes stored layer preferences and whether Trackball mode is selected.
struct CommandState {
    bool visible = true;
    bool enabled = true;
    bool checked = false;
    bool operator==(const CommandState&) const = default;
};
constexpr std::size_t kCommandCount = static_cast<std::size_t>(Command::CycleStyle) + 1;
using ToolbarState = std::array<CommandState, kCommandCount>;
ToolbarState toolbar_state(const Model& model);

enum class CursorIntent : std::uint8_t { Arrow, OpenHand, ClosedHand };
CursorIntent cursor_for(const Model& model, ScreenPoint pointer);
struct ButtonRect {
    int x, y, width, height;
};
constexpr int kToolbarWidth = 404, kToolbarHeight = 132;
ButtonRect toolbar_button(Command command);
std::optional<Command> toolbar_hit(ScreenPoint pointer);

// Two presses within kDoubleClickMs and kDoubleClickSlopPx, the second on the globe, zoom in by
// kDoubleClickZoom and center the globe on the clicked point.
constexpr std::uint64_t kDoubleClickMs = 400;
// A plain click picks only after this long without a second click, so a double click zooms and
// does not also open a card. It must not be shorter than kDoubleClickMs.
constexpr std::uint64_t kPickDelayMs = kDoubleClickMs;
// Overall refinement progress 0..1 given the running job's own fraction: the elevation levels run
// one job each, coarse to fine, so each level is an equal share. 1 when nothing is refining.
double refine_progress(const Model& model, double job_fraction);
constexpr double kDoubleClickSlopPx = 6.0;
constexpr double kDoubleClickZoom = 2.0;

// A press alone changes nothing: no stage change, no new generation, no redraw. A drag begins once
// the pointer is more than kDragSlopPx from where the button went down; the globe then follows the
// pointer from that press position, so nothing lags. A release without a drag leaves the model
// visibly untouched, and platforms treat it as a click (country picking).
constexpr double kDragSlopPx = 3.0;

constexpr std::uint32_t kSettleDelayMs = 150;
// While a drag is active (button down, past the slop) platforms keep showing the plain moving
// frame: a refinement started by a pause computes into the framebuffer but is not presented until
// release.
constexpr bool presentation_held(const Model& model) {
    return model.dragging && model.drag_active;
}
// A drag that pauses this long (button still down) starts refining; moving again cancels it, and
// releasing after it finished keeps the result, shown at once.
constexpr std::uint32_t kDragPauseMs = 300;
constexpr std::uint32_t kRefineDelayMs = 1; // separate event-loop turn for Settle work

Effects update(Model& model, const Msg& msg);

} // namespace globe
