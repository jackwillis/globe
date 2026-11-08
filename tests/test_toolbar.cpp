// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/app.hpp"
#include <cmath>
using namespace globe;
int main() {
    auto m = initial_model(640, 480);
    for (auto mode : {DragMode::Turntable, DragMode::Trackball}) {
        m.drag_mode = mode;
        CHECK(cursor_for(m, {320, 240}) == CursorIntent::OpenHand);
        CHECK(cursor_for(m, {0, 0}) == CursorIntent::Arrow);
        CHECK(cursor_for(m, {NAN, 0}) == CursorIntent::Arrow);
        CHECK(cursor_for(m, {320 + m.camera.radius_px, 240}) == CursorIntent::OpenHand);
        m.dragging = true;
        CHECK(cursor_for(m, {0, 0}) == CursorIntent::ClosedHand);
        m.dragging = false;
        // A place under the pointer, or a selected country (a click on the sea clears it), makes
        // the globe clickable: the pointing hand. Off the globe it is still the arrow, and a
        // drag in progress keeps the closed hand whatever is underneath.
        const CursorTarget place{false, true}, selected{true, false};
        for (const auto target : {place, selected, CursorTarget{true, true}}) {
            CHECK(cursor_for(m, {320, 240}, target) == CursorIntent::PointingHand);
            CHECK(cursor_for(m, {0, 0}, target) == CursorIntent::Arrow);
            CHECK(cursor_for(m, {NAN, 0}, target) == CursorIntent::Arrow);
            m.dragging = true;
            CHECK(cursor_for(m, {320, 240}, target) == CursorIntent::ClosedHand);
            m.dragging = false;
        }
        CHECK(cursor_for(m, {320, 240}, CursorTarget{}) == CursorIntent::OpenHand);
    }
    for (std::size_t i = 0; i < kCommandCount; ++i) {
        auto r = toolbar_button(static_cast<Command>(i));
        CHECK(toolbar_hit({double(r.x + 1), double(r.y + 1)}) == static_cast<Command>(i));
        CHECK(!toolbar_hit({double(r.x + r.width), double(r.y + 1)}));
    }
    CHECK(!toolbar_hit({-1, 0}));
    CHECK(!toolbar_hit({NAN, 0}));
    CHECK(!toolbar_hit({0, INFINITY}));
    CHECK(!toolbar_hit({10000, 10000}));
    return TEST_MAIN_RESULT();
}
