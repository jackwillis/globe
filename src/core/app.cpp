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

} // namespace

double min_radius_px(int width, int height) {
    return kMinRadiusFraction * short_side(width, height);
}

double max_radius_px(int width, int height) {
    return kMaxRadiusFraction * short_side(width, height);
}

Model initial_model(int width, int height) {
    width = std::clamp(width, 1, kMaxViewportDimension);
    height = std::clamp(height, 1, kMaxViewportDimension);
    Model m{};
    m.camera = home_camera(width, height);
    m.layers = Layers{};
    m.stage = QualityStage::Moving;
    m.dragging = false;
    m.drag_last = {0.0, 0.0};
    m.last_input_ms = 0;
    return m;
}

Effects update(Model& model, const Msg& msg) {
    Effects fx;
    if (const auto* down = std::get_if<PointerDown>(&msg)) {
        if (finite(down->x, down->y)) {
            model.dragging = true;
            model.drag_last = {down->x, down->y};
            model.last_input_ms = down->time_ms;
        }
    } else if (const auto* move = std::get_if<PointerMove>(&msg)) {
        if (model.dragging && finite(move->x, move->y)) {
            const ScreenPoint to{move->x, move->y};
            fx = set_camera(model, drag_rotate(model.camera, model.drag_last, to));
            model.drag_last = to;
            model.last_input_ms = move->time_ms;
        }
    } else if (const auto* up = std::get_if<PointerUp>(&msg)) {
        model.dragging = false;
        model.last_input_ms = up->time_ms;
    } else if (const auto* wheel = std::get_if<Wheel>(&msg)) {
        fx = set_camera(model, zoom_camera(model.camera, wheel->steps));
        model.last_input_ms = wheel->time_ms;
    } else if (const auto* key = std::get_if<KeyPress>(&msg)) {
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
    } else if (std::holds_alternative<Expose>(msg)) {
        fx.redraw = true;
    }
    return fx;
}

} // namespace globe
