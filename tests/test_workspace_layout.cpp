// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Geometry rules of the #148 workspace prototype (tools/workspace-prototype). Host only: it checks
// the integer rules, not how the Window Manager behaves.
#include "check.hpp"
#include "workspace_layout.hpp"

using namespace globe::workspace;

namespace {
Screen screen(int w, int h) {
    return Screen{Rect{0, 0, w, h}, 20};
}
bool inside(const Rect& r, const Screen& s) {
    const Rect d = desktop(s);
    const Rect f = structure(r);
    return f.left >= d.left && f.right <= d.right && f.top >= d.top && f.bottom <= d.bottom;
}
bool apart(const Rect& a, const Rect& b) {
    return overlap_area(structure(a), structure(b)) == 0;
}
} // namespace

int main() {
    // 640 x 480 is today's single-window content rectangle, so the globe loses nothing.
    const auto small = initial_arrangement(screen(640, 480));
    CHECK(small.mode == Mode::Compact);
    CHECK(small.globe == (Rect{6, 46, 634, 474}));
    CHECK(!small.info_open && !small.palette_open);
    CHECK(grabbable(small.info, screen(640, 480)) && grabbable(small.palette, screen(640, 480)));
    CHECK(inside(small.info, screen(640, 480)) && inside(small.palette, screen(640, 480)));
    // Beside, naively, at 640: the globe falls under the sidebar layout's 408-pixel pane.
    CHECK(globe_content(screen(640, 480), kGap + kInfoW).width() == 322);

    // The threshold: 726 pixels wide is the narrowest screen that opens beside.
    CHECK(initial_arrangement(screen(725, 600)).mode == Mode::Compact);
    CHECK(initial_arrangement(screen(726, 600)).mode == Mode::Beside);
    CHECK(initial_arrangement(screen(1024, 342)).mode == Mode::Compact); // too short for the column

    const auto large = initial_arrangement(screen(1024, 768));
    CHECK(large.mode == Mode::Beside && large.info_open && large.palette_open);
    CHECK(large.globe.width() == 706 && large.globe.height() == 716);

    // Every screen from a Mac Plus to a large display.
    for (int w = 512; w <= 2560; w += 16) {
        for (int h = 342; h <= 1600; h += 18) {
            const Screen s = screen(w, h);
            const auto a = initial_arrangement(s);
            CHECK(a.globe.width() >= kMinGlobeW && a.globe.height() >= kMinGlobeH);
            CHECK(a.globe.width() <= kMaxGlobeW && a.globe.height() <= kMaxGlobeH);
            CHECK(inside(a.globe, s) || a.globe.width() == kMinGlobeW ||
                  a.globe.height() == kMinGlobeH);
            CHECK(grabbable(a.globe, s));
            CHECK(grabbable(a.info, s) && grabbable(a.palette, s));
            if (a.mode == Mode::Beside) {
                CHECK(a.globe.width() >= kBaselineGlobeW);
                CHECK(inside(a.info, s) && inside(a.palette, s));
                CHECK(apart(a.globe, a.info) && apart(a.globe, a.palette) &&
                      apart(a.info, a.palette));
            } else {
                CHECK(!a.info_open && !a.palette_open);
                CHECK(inside(a.info, s) || a.info.width() == kInfoMinW);
            }
        }
    }

    // Recovery: from anywhere, a window comes back grabbable; a sensible position is untouched.
    const Screen s = screen(640, 480);
    const Rect fine{100, 100, 400, 300};
    CHECK(recover(fine, s, kInfoMinW, kInfoMinH) == fine);
    for (int x = -2000; x <= 2000; x += 97) {
        for (int y = -2000; y <= 2000; y += 89) {
            for (int size : {160, 300, 3000}) {
                const Rect lost{x, y, x + size, y + size};
                const Rect back = recover(lost, s, kInfoMinW, kInfoMinH);
                CHECK(grabbable(back, s));
                CHECK(back.width() >= kInfoMinW && back.height() >= kInfoMinH);
                CHECK(recover(back, s, kInfoMinW, kInfoMinH) == back); // idempotent
            }
        }
    }
    // Under the menu bar, or with only a sliver on screen, is not grabbable.
    CHECK(!grabbable(Rect{100, 20, 400, 200}, s));  // title bar would be inside the menu bar
    CHECK(!grabbable(Rect{630, 100, 930, 300}, s)); // 11 px of title bar on screen
    CHECK(!grabbable(Rect{600, 100, 900, 300}, s)); // 41 px on screen, short of the 64 needed
    CHECK(grabbable(Rect{560, 100, 860, 300}, s));  // 81 px on screen
    return TEST_MAIN_RESULT();
}
