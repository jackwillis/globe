// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Prints the #148 prototype's geometry as "key left top right bottom" lines for render_mockups.py,
// so the mockups and the tested rules share one source of numbers.
#include "workspace_layout.hpp"
#include <cstdio>

using namespace globe::workspace;

namespace {
void rect(const char* scene, const char* key, const Rect& r) {
    std::printf("%s %s %d %d %d %d\n", scene, key, r.left, r.top, r.right, r.bottom);
}
void arrangement(const char* scene, const Arrangement& a) {
    rect(scene, "globe", a.globe);
    rect(scene, "info", a.info);
    rect(scene, "palette", a.palette);
    std::printf("%s open %d %d\n", scene, a.info_open, a.palette_open);
}
} // namespace

int main() {
    const Screen s640{Rect{0, 0, 640, 480}, 20}, s1024{Rect{0, 0, 1024, 768}, 20};
    arrangement("s640", initial_arrangement(s640));
    arrangement("s1024", initial_arrangement(s1024));
    // What "beside" would cost at 640 x 480: the column takes 306 pixels from the globe.
    const Rect naive = globe_content(s640, kGap + kInfoW);
    rect("s640naive", "globe", naive);
    rect("s640naive", "info", column_info(naive));
    rect("s640naive", "palette", column_palette(naive));
    // The sidebar layout (today): the 220-pixel sidebar inside the one window.
    const Rect single = globe_content(s640, 0);
    rect("s640sidebar", "window", single);
    rect("s1024sidebar", "window", globe_content(s1024, 0));
    // Recovery cases on 640 x 480 (Information-sized windows): lost -> recovered.
    const struct {
        const char* name;
        Rect lost;
    } cases[] = {
        {"offright", {900, 200, 1200, 400}}, {"offbottom", {200, 700, 500, 900}},
        {"offleft", {-600, 200, -300, 400}}, {"undermenu", {100, 0, 400, 200}},
        {"sliver", {630, 100, 930, 300}},    {"huge", {-100, -100, 2000, 1500}},
    };
    for (const auto& c : cases) {
        char key[32];
        std::snprintf(key, sizeof key, "lost-%s", c.name);
        rect("recover", key, c.lost);
        std::snprintf(key, sizeof key, "back-%s", c.name);
        rect("recover", key, recover(c.lost, s640, kInfoMinW, kInfoMinH));
    }
    return 0;
}
