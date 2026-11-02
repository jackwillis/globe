// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/app.hpp"

#include <algorithm>
#include <cmath>

#include "globe/trackball.hpp"

namespace globe {

namespace {

constexpr LatLon kHome = {20.0, 0.0};
constexpr double kHomeRadiusFraction = 0.4; // of the shorter viewport side
constexpr double kMinRadiusFraction = 0.2;
constexpr double kMaxRadiusFraction = 8.0;
constexpr double kZoomStep = 1.1;  // per wheel step or key press
constexpr int kMaxWheelSteps = 40; // 1.1^40 > 40: one message can cross the whole zoom range
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

Effects on_key(Model& model, Key key) {
    const Camera& cam = model.camera;
    const double d = cam.radius_px * kSin10Degrees;
    const ScreenPoint c{cam.center_x, cam.center_y};
    Effects fx;
    switch (key) {
    case Key::Left:
        return set_camera(model, drag_rotate(cam, c, {c.x - d, c.y}));
    case Key::Right:
        return set_camera(model, drag_rotate(cam, c, {c.x + d, c.y}));
    case Key::Up:
        return set_camera(model, drag_rotate(cam, c, {c.x, c.y - d}));
    case Key::Down:
        return set_camera(model, drag_rotate(cam, c, {c.x, c.y + d}));
    case Key::ZoomIn:
        return set_camera(model, zoom_camera(cam, 1));
    case Key::ZoomOut:
        return set_camera(model, zoom_camera(cam, -1));
    case Key::Reset:
        return set_camera(model, home_camera(cam.width, cam.height));
    case Key::ToggleGraticule:
        model.layers.graticule = !model.layers.graticule;
        fx.redraw = true;
        return fx;
    case Key::ToggleBorders:
        model.layers.borders = !model.layers.borders;
        fx.redraw = true;
        return fx;
    case Key::ToggleCoastlines:
        model.layers.coastlines = !model.layers.coastlines;
        fx.redraw = true;
        return fx;
    case Key::Quit:
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
    if (model.dragging) {
        layers.graticule = false;
    }
    return layers;
}

Model initial_model(int width, int height) {
    width = std::clamp(width, 1, kMaxViewportDimension);
    height = std::clamp(height, 1, kMaxViewportDimension);
    Model m{};
    m.camera = home_camera(width, height);
    m.layers = Layers{};
    m.stage = QualityStage::Moving;
    m.dragging = false;
    m.drag_moved = false;
    m.drag_last = {0.0, 0.0};
    m.click_valid = false;
    m.click_pos = {0.0, 0.0};
    m.click_ms = 0;
    m.last_input_ms = 0;
    return m;
}

Effects update(Model& model, const Msg& msg) {
    Effects fx;
    if (const auto* down = std::get_if<PointerDown>(&msg)) {
        if (finite(down->x, down->y)) {
            const ScreenPoint p{down->x, down->y};
            const bool second_click =
                model.click_valid && down->time_ms >= model.click_ms &&
                down->time_ms - model.click_ms <= kDoubleClickMs &&
                distance_squared(p, model.click_pos) <= kDoubleClickSlopPx * kDoubleClickSlopPx;
            model.last_input_ms = down->time_ms;
            if (second_click && on_globe(model.camera, p)) {
                // Bring the clicked point to the center, then zoom in on it.
                model.click_valid = false;
                model.dragging = false;
                model.drag_moved = false;
                const Camera centered =
                    drag_rotate(model.camera, p, {model.camera.center_x, model.camera.center_y});
                fx = set_camera(model, zoomed(centered, kDoubleClickZoom,
                                              min_radius_px(centered.width, centered.height),
                                              max_radius_px(centered.width, centered.height)));
            } else {
                model.dragging = true;
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
            if (model.click_valid &&
                distance_squared(to, model.click_pos) > kDoubleClickSlopPx * kDoubleClickSlopPx) {
                model.click_valid = false; // it is a drag, not a click
            }
            fx = set_camera(model, drag_rotate(model.camera, model.drag_last, to));
            model.drag_moved |= fx.redraw;
            model.drag_last = to;
            model.last_input_ms = move->time_ms;
        }
    } else if (const auto* up = std::get_if<PointerUp>(&msg)) {
        // The graticule was hidden while dragging; bring it back, but only if the drag drew.
        fx.redraw = model.dragging && model.drag_moved && model.layers.graticule;
        model.dragging = false;
        model.drag_moved = false;
        model.last_input_ms = up->time_ms;
    } else if (const auto* wheel = std::get_if<Wheel>(&msg)) {
        fx = set_camera(model, zoom_camera(model.camera, wheel->steps));
        model.last_input_ms = wheel->time_ms;
    } else if (const auto* key = std::get_if<KeyDown>(&msg)) {
        fx = on_key(model, key->key);
        model.last_input_ms = key->time_ms;
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
            fx.redraw = true;
        }
    } else if (std::holds_alternative<Repaint>(msg)) {
        fx.redraw = true;
    }
    return fx;
}

} // namespace globe
