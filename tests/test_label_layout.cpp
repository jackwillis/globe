// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include <array>
#include <cmath>
#include <limits>

#include "check.hpp"
#include "globe/label_layout.hpp"

using namespace globe;

namespace {

std::optional<LabelMetrics> measure(void* context, std::string_view text, LabelRole role) {
    ++*static_cast<int*>(context);
    if (text == "missing") {
        return std::nullopt;
    }
    if (text == "invalid") {
        return LabelMetrics{0, 4, std::numeric_limits<int>::max(), 2};
    }
    return LabelMetrics{-2, static_cast<int>(text.size()) * (role == LabelRole::area ? 5 : 3) - 2,
                        6, 2};
}

// An axis-aligned camera lets the fixture specify screen coordinates without trig.
Vec3 anchor(const Camera& camera, int x, int y) {
    const double dx = (x - camera.center_x) / camera.radius_px;
    const double dy = (camera.center_y - y) / camera.radius_px;
    return {dx, dy, std::sqrt(1 - dx * dx - dy * dy)};
}

} // namespace

int main() {
    const Camera camera{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, 64, 64, 64, 100, 128};
    const auto at = [&](int x, int y, std::string_view text, int priority,
                        LabelRole role = LabelRole::area) {
        return LabelCandidate{anchor(camera, x, y), text, role, priority};
    };
    const std::array<LabelCandidate, 11> candidates{
        at(64, 64, "same", 1), // Overlapped by the higher priority label.
        at(64, 64, "map!", 10),
        at(84, 64, "edge", 10), // Touching boxes are allowed; tie uses input order.
        at(99, 48, "view", 20), // Only the box exceeds the viewport.
        at(39, 8, "x", 20),     // Only a corner exceeds the limb.
        LabelCandidate{{0, 0, -1}, "back", LabelRole::area, 20},
        at(52, 94, "A long label which cannot fit inside this view", 20),
        at(30, 64, "missing", 20),
        at(30, 64, "invalid", 20),
        at(30, 64, "p", 0, LabelRole::point),
        at(104, 64, "off", 20), // Anchor outside the viewport.
    };
    constexpr std::array expected{
        PlacedLabel{1, 56, 66, {54, 60, 74, 68}},
        PlacedLabel{2, 76, 66, {74, 60, 94, 68}},
        PlacedLabel{9, 31, 66, {29, 60, 32, 68}},
    };

    std::array<PlacedLabel, 4> placed{};
    int calls = 0;
    CHECK(place_labels(camera, candidates, measure, &calls, placed) == expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        CHECK(placed[i] == expected[i]);
    }
    CHECK(calls == 9);
    CHECK(placed.back() == PlacedLabel{});

    calls = 0;
    CHECK(place_labels(camera, candidates, measure, &calls, std::span(placed).first(1)) == 1);
    CHECK(placed[0] == expected[0]);
    CHECK(calls == 6);
    CHECK(place_labels(camera, candidates, measure, &calls, {}) == 0);
    CHECK(calls == 6);

    std::array<LabelCandidate, kMaxLabelCandidates + 1> many{};
    std::array<PlacedLabel, kMaxPlacedLabels + 1> large_output{};
    CHECK(place_labels(camera, many, measure, &calls, large_output) == 0);
    CHECK(calls == 6);

    for (std::size_t i = 0; i < large_output.size(); ++i) {
        many[i] = at(24 + static_cast<int>(i % 7) * 10, 32 + static_cast<int>(i / 7) * 10, "p", 0,
                     LabelRole::point);
    }
    calls = 0;
    CHECK(place_labels(camera, std::span(many).first(kMaxLabelCandidates), measure, &calls,
                       large_output) == kMaxPlacedLabels);
    CHECK(calls == static_cast<int>(kMaxPlacedLabels));
    CHECK(large_output.back() == PlacedLabel{});
    return TEST_MAIN_RESULT();
}
