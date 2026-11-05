// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/app.hpp"
#include "globe/elevation.hpp"

#include <algorithm>
#include <cmath>

#include "globe/trackball.hpp"

namespace globe {

namespace {

constexpr LatLon kHome = {20.0, 0.0};
constexpr double kHomeRadiusFraction = 0.4; // of the shorter viewport side
constexpr double kMinRadiusFraction = 0.2;
constexpr double kMaxRadiusFraction = 8.0;
constexpr double kZoomStep = 1.1;  // per wheel step or command press
constexpr int kMaxWheelSteps = 40; // 1.1^40 > 40: one message can cross the whole zoom range
constexpr double kMaxTurntableLatitude = 89.9; // keeps the longitude defined at the pole
constexpr double kArrowStepDegrees = 10.0;
constexpr double kSin10Degrees = 0.17364817766693033; // arrow keys turn the globe about 10 degrees

double short_side(int width, int height) {
    return static_cast<double>(std::min(width, height));
}

Camera home_camera(int width, int height) {
    const auto cam =
        make_camera({kHome, 0.0, kHomeRadiusFraction * short_side(width, height), width, height});
    // Dimensions were clamped by the caller, so construction cannot fail; the fallback keeps the
    // function total without exceptions.
    return cam.value_or(Camera{});
}

Camera zoom_camera(const Camera& cam, int steps) {
    double factor = 1.0;
    const int n = std::min(std::abs(steps), kMaxWheelSteps);
    for (int i = 0; i < n; ++i) {
        factor *= kZoomStep;
    }
    if (steps < 0) {
        factor = 1.0 / factor;
    }
    return zoomed(cam, factor, min_radius_px(cam.width, cam.height),
                  max_radius_px(cam.width, cam.height));
}

// A camera looking at `center` with north up, at `like`'s radius and viewport.
Camera north_up_camera(const Camera& like, LatLon center) {
    center.lat_deg = std::clamp(center.lat_deg, -kMaxTurntableLatitude, kMaxTurntableLatitude);
    const auto cam = make_camera({center, 0.0, like.radius_px, like.width, like.height});
    return cam.value_or(like);
}

// Constant angle per pixel (one pixel is 1/radius radians), so zooming in slows the turn and
// the rim of the globe behaves like the middle. Dragging right turns the globe east to west.
Camera turntable_drag(const Camera& cam, ScreenPoint from, ScreenPoint to) {
    if (to.x == from.x && to.y == from.y) {
        return cam;
    }
    LatLon c = center_of(cam);
    const double deg_per_px = kRadToDeg / cam.radius_px;
    c.lon_deg -= (to.x - from.x) * deg_per_px;
    c.lat_deg += (to.y - from.y) * deg_per_px;
    return north_up_camera(cam, c);
}

Camera drag_camera(const Model& model, ScreenPoint from, ScreenPoint to) {
    return model.drag_mode == DragMode::Turntable ? turntable_drag(model.camera, from, to)
                                                  : drag_rotate(model.camera, from, to);
}

// Moves the view by whole degrees of latitude/longitude, keeping north up.
Camera turntable_step(const Camera& cam, double dlon, double dlat) {
    LatLon c = center_of(cam);
    c.lon_deg += dlon;
    c.lat_deg += dlat;
    return north_up_camera(cam, c);
}

bool same_view(const Camera& a, const Camera& b) {
    return a.radius_px == b.radius_px && a.right.x == b.right.x && a.right.y == b.right.y &&
           a.right.z == b.right.z && a.up.x == b.up.x && a.up.y == b.up.y && a.up.z == b.up.z &&
           a.forward.x == b.forward.x && a.forward.y == b.forward.y && a.forward.z == b.forward.z;
}

// Applies a new camera and reports whether anything visible changed.
Effects set_camera(Model& model, const Camera& next) {
    Effects fx;
    if (!same_view(model.camera, next)) {
        model.camera = next;
        model.stage = QualityStage::Moving;
        fx.redraw = true;
    }
    return fx;
}

Effects on_command(Model& model, Command command) {
    const Camera& cam = model.camera;
    const double d = cam.radius_px * kSin10Degrees;
    const ScreenPoint c{cam.center_x, cam.center_y};
    Effects fx;
    const bool turntable = model.drag_mode == DragMode::Turntable;
    switch (command) {
    case Command::Left:
        return set_camera(model, turntable ? turntable_step(cam, kArrowStepDegrees, 0.0)
                                           : drag_rotate(cam, c, {c.x - d, c.y}));
    case Command::Right:
        return set_camera(model, turntable ? turntable_step(cam, -kArrowStepDegrees, 0.0)
                                           : drag_rotate(cam, c, {c.x + d, c.y}));
    case Command::Up:
        return set_camera(model, turntable ? turntable_step(cam, 0.0, -kArrowStepDegrees)
                                           : drag_rotate(cam, c, {c.x, c.y - d}));
    case Command::Down:
        return set_camera(model, turntable ? turntable_step(cam, 0.0, kArrowStepDegrees)
                                           : drag_rotate(cam, c, {c.x, c.y + d}));
    case Command::AlignNorth:
        return set_camera(model, north_up_camera(cam, center_of(cam)));
    case Command::ToggleDragMode:
        model.drag_mode = turntable ? DragMode::Trackball : DragMode::Turntable;
        return fx;
    case Command::ZoomIn:
        return set_camera(model, zoom_camera(cam, 1));
    case Command::ZoomOut:
        return set_camera(model, zoom_camera(cam, -1));
    case Command::Reset:
        return set_camera(model, home_camera(cam.width, cam.height));
    case Command::ToggleGraticule:
        model.layers.graticule = !model.layers.graticule;
        fx.redraw = true;
        return fx;
    case Command::ToggleBorders:
        model.layers.borders = !model.layers.borders;
        fx.redraw = true;
        return fx;
    case Command::CycleStyle:
        model.terrain_style = next_terrain_style(model.terrain_style);
        fx.redraw = true;
        return fx;
    case Command::Quit:
        fx.quit = true;
        return fx;
    }
    return fx;
}

bool finite(double x, double y) {
    return std::isfinite(x) && std::isfinite(y);
}

// Squared distances: no square root (slow on some targets) and no std::hypot, which Retro68's
// math library lacks.
double distance_squared(ScreenPoint a, ScreenPoint b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return dx * dx + dy * dy;
}

bool on_globe(const Camera& cam, ScreenPoint p) {
    return distance_squared(p, {cam.center_x, cam.center_y}) <= cam.radius_px * cam.radius_px;
}

} // namespace

double min_radius_px(int width, int height) {
    return kMinRadiusFraction * short_side(width, height);
}

double max_radius_px(int width, int height) {
    return kMaxRadiusFraction * short_side(width, height);
}

Layers effective_layers(const Model& model) {
    Layers layers = model.layers;
    if (model.dragging && model.drag_active && model.stage == QualityStage::Moving) {
        layers.graticule = false; // a paused drag that has begun refining shows it
    }
    return layers;
}

RenderQuality quality_for(const Model& model) {
    const bool fine = model.stage == QualityStage::Refine || model.stage == QualityStage::Decorate;
    unsigned selected = 0;
    std::uint32_t best_width = 0;
    bool meets_target = false;
    for (unsigned i = 0; i < kElevationLevels; ++i) {
        const auto width = model.elevations[i].width;
        if (width == 0)
            continue;
        const bool meets = model.camera.radius_px * (360.0 * kDegToRad) / width <= 4.0;
        if ((meets && (!meets_target || width <= best_width)) ||
            (!meets && !meets_target && width >= best_width)) {
            selected = i;
            best_width = width;
            meets_target = meets;
        }
    }
    if (model.refine_active)
        selected = model.refine_level;
    RenderQuality quality{fine ? 1.5 : 3.0, fine, selected, model.terrain_style};
    quality.preserve_bitmap = model.refine_active && !model.refine_first;
    quality.terrain_only = model.refine_active && model.refine_level != model.refine_target;
    // Refinement always radiates from the view center.
    quality.focus = {model.camera.center_x, model.camera.center_y};
    return quality;
}

// Choose the next distinct grid width, resolving equal widths in favor of later imports.
static unsigned next_resolution(const Model& model, std::uint32_t after, std::uint32_t target) {
    unsigned chosen = kElevationLevels;
    std::uint32_t width = 0;
    for (unsigned i = 0; i < kElevationLevels; ++i) {
        const auto candidate = model.elevations[i].width;
        if (candidate > after && candidate <= target && (width == 0 || candidate <= width)) {
            width = candidate;
            chosen = i;
        }
    }
    return chosen;
}
Effects start_refinement(Model& model) {
    Effects fx;
    if (model.refine_active)
        return fx;
    model.stage = QualityStage::Refine;
    model.refine_target = quality_for(model).elevation_level;
    const auto first = next_resolution(model, 0, model.elevations[model.refine_target].width);
    model.refine_level = first < kElevationLevels ? first : model.refine_target;
    model.refine_active = true;
    model.refine_first = true;
    fx.refine = Refine{model.generation, model.refine_level};
    return fx;
}

CursorIntent cursor_for(const Model& model, ScreenPoint pointer) {
    if (model.dragging)
        return CursorIntent::ClosedHand;
    const double x = pointer.x - model.camera.center_x;
    const double y = pointer.y - model.camera.center_y;
    return std::isfinite(x) && std::isfinite(y) &&
                   x * x + y * y <= model.camera.radius_px * model.camera.radius_px
               ? CursorIntent::OpenHand
               : CursorIntent::Arrow;
}
ButtonRect toolbar_button(Command command) {
    const int i = static_cast<int>(command);
    return {4 + (i % 4) * 100, 4 + (i / 4) * 32, 96, 28};
}
std::optional<Command> toolbar_hit(ScreenPoint pointer) {
    if (!std::isfinite(pointer.x) || !std::isfinite(pointer.y))
        return std::nullopt;
    for (std::size_t i = 0; i < kCommandCount; ++i) {
        const auto r = toolbar_button(static_cast<Command>(i));
        if (pointer.x >= r.x && pointer.x < r.x + r.width && pointer.y >= r.y &&
            pointer.y < r.y + r.height)
            return static_cast<Command>(i);
    }
    return std::nullopt;
}

ToolbarState toolbar_state(const Model& model) {
    ToolbarState state{};
    const auto index = [](Command command) { return static_cast<std::size_t>(command); };
    state[index(Command::ToggleGraticule)].checked = model.layers.graticule;
    state[index(Command::ToggleBorders)].checked = model.layers.borders;
    state[index(Command::ToggleDragMode)].checked = model.drag_mode == DragMode::Trackball;
    state[index(Command::CycleStyle)].checked = model.terrain_style == TerrainStyle::Relief;
    // World north is +Y. Its screen projection must point up, with no sideways component.
    state[index(Command::AlignNorth)].enabled =
        std::fabs(model.camera.right.y) > 1e-12 || model.camera.up.y < 0.0;
    state[index(Command::ZoomIn)].enabled =
        model.camera.radius_px < max_radius_px(model.camera.width, model.camera.height);
    state[index(Command::ZoomOut)].enabled =
        model.camera.radius_px > min_radius_px(model.camera.width, model.camera.height);
    return state;
}

Model initial_model(int width, int height) {
    width = std::clamp(width, 1, kMaxViewportDimension);
    height = std::clamp(height, 1, kMaxViewportDimension);
    Model m{};
    m.camera = home_camera(width, height);
    m.layers = Layers{};
    m.drag_mode = DragMode::Turntable;
    m.stage = QualityStage::Moving;
    m.stage_needs_time = true;
    m.dragging = false;
    m.drag_active = false;
    m.drag_moved = false;
    m.drag_last = {0.0, 0.0};
    m.click_valid = false;
    m.click_pos = {0.0, 0.0};
    m.click_ms = 0;
    m.last_input_ms = 0;
    m.pick_pending = false;
    m.pick_pos = {0.0, 0.0};
    m.pick_due_ms = 0;
    return m;
}

Effects update(Model& model, const Msg& msg) {
    Effects fx;
    const auto initial_generation = model.generation;
    bool input = false;
    std::uint64_t input_ms = 0;
    if (const auto* down = std::get_if<PointerDown>(&msg)) {
        if (finite(down->x, down->y)) {
            const ScreenPoint p{down->x, down->y};
            const bool second_click =
                model.click_valid && down->time_ms >= model.click_ms &&
                down->time_ms - model.click_ms <= kDoubleClickMs &&
                distance_squared(p, model.click_pos) <= kDoubleClickSlopPx * kDoubleClickSlopPx;
            model.last_input_ms = down->time_ms;
            model.pick_pending = false; // a second press (or a drag) cancels the waiting pick
            if (second_click && on_globe(model.camera, p)) {
                // Bring the clicked point to the center, then zoom in on it.
                model.click_valid = false;
                model.dragging = false;
                model.drag_active = false;
                model.drag_moved = false;
                Camera centered = model.camera;
                if (model.drag_mode == DragMode::Turntable) {
                    const Vec3 under = unproject_to_sphere(model.camera, p);
                    const Vec3 world = model.camera.right * under.x + model.camera.up * under.y +
                                       model.camera.forward * under.z;
                    centered = north_up_camera(model.camera, from_unit_vector(world));
                } else {
                    centered = drag_rotate(model.camera, p,
                                           {model.camera.center_x, model.camera.center_y});
                }
                fx = set_camera(model, zoomed(centered, kDoubleClickZoom,
                                              min_radius_px(centered.width, centered.height),
                                              max_radius_px(centered.width, centered.height)));
                input = fx.redraw; // a zoom that changed nothing is not input
                input_ms = down->time_ms;
            } else {
                // A press alone is not input: the settled frame stays, see kDragSlopPx.
                model.dragging = true;
                model.drag_active = false;
                model.drag_moved = false;
                model.drag_last = p;
                model.click_valid = true;
                model.click_pos = p;
                model.click_ms = down->time_ms;
            }
        }
    } else if (const auto* move = std::get_if<PointerMove>(&msg)) {
        if (model.dragging && finite(move->x, move->y)) {
            const ScreenPoint to{move->x, move->y};
            // drag_last stays at the press position until the slop is passed.
            if (!model.drag_active &&
                distance_squared(to, model.drag_last) > kDragSlopPx * kDragSlopPx) {
                model.drag_active = true;
                model.click_valid = false; // it is a drag, not a click
            }
            if (model.drag_active) {
                fx = set_camera(model, drag_camera(model, model.drag_last, to));
                model.drag_moved |= fx.redraw;
                model.drag_last = to;
                input = true;
                input_ms = move->time_ms;
                model.last_input_ms = move->time_ms;
            }
        }
    } else if (const auto* up = std::get_if<PointerUp>(&msg)) {
        // The graticule was hidden while dragging; bring it back, but only if the drag drew.
        // If the drag paused long enough to start refining, that work stands: release changes
        // nothing.
        const bool refining = model.stage != QualityStage::Moving;
        fx.redraw = model.dragging && model.drag_moved && model.layers.graticule && !refining;
        // Only a drag that changed the view restarts the settle clock; a plain click does not.
        input = model.dragging && model.drag_moved && !refining;
        if (model.dragging && !model.drag_moved) {
            // A plain click: pick after the double-click window, unless a second click comes.
            // While settling, the settle wakes already serve the pending pick (see Tick).
            model.pick_pending = true;
            model.pick_pos = model.drag_last;
            model.pick_due_ms = up->time_ms + kPickDelayMs;
            if (model.stage == QualityStage::Refine)
                fx.wake_after_ms = static_cast<std::uint32_t>(kPickDelayMs);
        }
        model.dragging = false;
        model.drag_active = false;
        model.drag_moved = false;
        input_ms = up->time_ms;
        if (input) {
            model.last_input_ms = up->time_ms;
        }
    } else if (const auto* wheel = std::get_if<Wheel>(&msg)) {
        fx = set_camera(model, zoom_camera(model.camera, wheel->steps));
        input = true;
        input_ms = wheel->time_ms;
        model.last_input_ms = wheel->time_ms;
    } else if (const auto* command = std::get_if<CommandIssued>(&msg)) {
        fx = on_command(model, command->command);
        input = true;
        input_ms = command->time_ms;
        model.last_input_ms = command->time_ms;
    } else if (const auto* resize = std::get_if<Resize>(&msg)) {
        if (resize->width >= 1 && resize->height >= 1 && resize->width <= kMaxViewportDimension &&
            resize->height <= kMaxViewportDimension &&
            (resize->width != model.camera.width || resize->height != model.camera.height)) {
            Camera cam = model.camera;
            // Keep the globe the same fraction of the window, within the new size's limits.
            const double scale = short_side(resize->width, resize->height) /
                                 short_side(model.camera.width, model.camera.height);
            cam.radius_px =
                std::clamp(cam.radius_px * scale, min_radius_px(resize->width, resize->height),
                           max_radius_px(resize->width, resize->height));
            cam.width = resize->width;
            cam.height = resize->height;
            cam.center_x = resize->width / 2.0;
            cam.center_y = resize->height / 2.0;
            model.camera = cam;
            model.pick_pending = false;
            model.stage = QualityStage::Moving;
            model.stage_needs_time = true;
            ++model.generation;
            fx.wake_after_ms = kSettleDelayMs;
            fx.redraw = true;
        }
    } else if (const auto* tick = std::get_if<Tick>(&msg)) {
        if (model.stage_needs_time) {
            model.stage_since_ms = tick->time_ms;
            model.stage_needs_time = false;
        }
        if (model.stage == QualityStage::Moving || model.stage == QualityStage::Settle) {
            const std::uint32_t delay =
                model.stage == QualityStage::Moving
                    ? (model.dragging && model.drag_active ? kDragPauseMs : kSettleDelayMs)
                    : kRefineDelayMs;
            const std::uint64_t elapsed =
                tick->time_ms >= model.stage_since_ms ? tick->time_ms - model.stage_since_ms : 0;
            if (elapsed < delay) {
                fx.wake_after_ms = delay - static_cast<std::uint32_t>(elapsed);
            } else if (model.stage == QualityStage::Moving) {
                model.stage = QualityStage::Settle;
                model.stage_since_ms = tick->time_ms;
                fx.wake_after_ms = kRefineDelayMs;
            } else {
                fx.refine = start_refinement(model).refine;
            }
        }
        if (model.pick_pending) {
            if (tick->time_ms >= model.pick_due_ms) {
                model.pick_pending = false;
                fx.pick = true;
                fx.pick_at = model.pick_pos;
            } else {
                const auto left = static_cast<std::uint32_t>(model.pick_due_ms - tick->time_ms);
                fx.wake_after_ms = fx.wake_after_ms != 0 ? std::min(fx.wake_after_ms, left) : left;
            }
        }
    } else if (const auto* done = std::get_if<RefinementFinished>(&msg)) {
        if (model.refine_active && done->generation == model.generation &&
            done->level == model.refine_level) {
            const auto next = next_resolution(model, model.elevations[model.refine_level].width,
                                              model.elevations[model.refine_target].width);
            if (next < kElevationLevels) {
                model.refine_level = next;
                model.refine_first = false;
                fx.refine = Refine{model.generation, next};
            } else {
                model.refine_active = false;
            }
        }
    } else if (const auto* pack = std::get_if<RegisterElevation>(&msg)) {
        if (pack->level < kElevationLevels && pack->height > 0 && pack->width <= 32768 &&
            pack->width == 2 * static_cast<std::uint64_t>(pack->height) &&
            static_cast<std::uint64_t>(pack->width) * pack->height <=
                static_cast<std::uint64_t>(kElevationMaxCells) &&
            (model.elevations[pack->level].width != pack->width ||
             model.elevations[pack->level].height != pack->height)) {
            model.elevations[pack->level] = {pack->width, pack->height};
            ++model.generation;
            model.stage = QualityStage::Moving;
            model.stage_needs_time = true;
            fx.redraw = true;
            fx.wake_after_ms = kSettleDelayMs;
        }
    } else if (std::holds_alternative<Repaint>(msg)) {
        fx.present_only = true; // the model is unchanged: show the existing pixels again
    }
    if (input) {
        model.pick_pending = false; // the view changed: a waiting pick is stale
        ++model.generation;
        model.stage = QualityStage::Moving;
        model.stage_since_ms = input_ms;
        model.stage_needs_time = false;
        fx.wake_after_ms = kSettleDelayMs;
    }
    if (model.generation != initial_generation)
        model.refine_active = false;
    return fx;
}

} // namespace globe
