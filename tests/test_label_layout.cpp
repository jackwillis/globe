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

    // A label beside its marker sits to the right with a gap, keeps clear of neighbours and of
    // labels an earlier call placed.
    const auto beside = [&](int x, int y, std::string_view text, int priority) {
        return LabelCandidate{anchor(camera, x, y), text, LabelRole::point, priority,
                              LabelSide::right};
    };
    std::array<PlacedLabel, 4> side_out{};
    const std::array lone{beside(64, 64, "dot", 0)};
    CHECK(place_labels(camera, lone, measure, &calls, side_out) == 1);
    CHECK(side_out[0] == (PlacedLabel{0, 70, 66, {68, 60, 77, 68}}));
    // 77 is the first free column; a box starting at 78 is within the two pixel gap.
    const std::array crowded{beside(64, 64, "dot", 2), beside(74, 64, "dot", 1),
                             beside(75, 64, "dot", 0)};
    side_out = {};
    CHECK(place_labels(camera, crowded, measure, &calls, side_out) == 2);
    CHECK(side_out[0].candidate == 0 && side_out[1].candidate == 2);
    std::array<PlacedLabel, 4> first_pass{};
    CHECK(place_labels(camera, std::span(candidates).first(2), measure, &calls, first_pass) == 1);
    side_out = {};
    CHECK(place_labels(camera, lone, measure, &calls, side_out, std::span(first_pass).first(1)) ==
          0);
    const std::array<PlacedLabel, 1> elsewhere{PlacedLabel{0, 0, 0, {0, 0, 4, 4}}};
    CHECK(place_labels(camera, lone, measure, &calls, side_out, elsewhere) == 1);

    // A centred label ducks the boxes in `avoid`: up first, then down, right, left; with nowhere
    // clear it stays put. Overlap with another label still drops it. "dot" is 7+2 wide, 8 high.
    const auto centred = [&](int x, int y, std::string_view text, int priority) {
        return LabelCandidate{anchor(camera, x, y), text, LabelRole::point, priority};
    };
    const std::array<PlacedLabel, 1> dot{PlacedLabel{0, 0, 0, {60, 60, 68, 68}}};
    const std::array<LabelCandidate, 1> one{centred(64, 64, "dot", 0)};
    std::array<PlacedLabel, 2> duck{};
    CHECK(place_labels(camera, one, measure, &calls, duck, dot) == 1);
    CHECK(duck[0].box == (LabelBox{60, 52, 69, 60})); // up by 4 + 4, touching the dot's top edge
    const std::array<PlacedLabel, 2> up_taken{dot[0], PlacedLabel{0, 0, 0, {58, 50, 72, 61}}};
    CHECK(place_labels(camera, one, measure, &calls, duck, up_taken) == 1);
    CHECK(duck[0].box == (LabelBox{60, 68, 69, 76})); // up is taken, so down
    const std::array<PlacedLabel, 3> boxed{dot[0], PlacedLabel{0, 0, 0, {58, 50, 72, 59}},
                                           PlacedLabel{0, 0, 0, {58, 69, 72, 80}}};
    CHECK(place_labels(camera, one, measure, &calls, duck, boxed) == 1);
    CHECK(duck[0].box == (LabelBox{68, 60, 77, 68})); // up and down taken, so right
    const std::array<PlacedLabel, 1> everywhere{PlacedLabel{0, 0, 0, {0, 0, 128, 128}}};
    CHECK(place_labels(camera, one, measure, &calls, duck, everywhere) == 1);
    CHECK(duck[0].box == (LabelBox{60, 60, 69, 68})); // nowhere clear: stays centred

    // Wrapping: a name wider than max_width breaks at spaces, at most three lines; centred labels
    // centre each line, labels beside a dot align left; a long single word stays whole.
    const auto wrapping = [&](int x, int y, std::string_view text, LabelRole role, LabelSide side,
                              int max_width) {
        return LabelCandidate{anchor(camera, x, y), text, role, 0, side, max_width};
    };
    const auto run_one = [&](const LabelCandidate& candidate, PlacedLabel& result) {
        const std::array one_label{candidate};
        std::array<PlacedLabel, 1> out{};
        const auto n = place_labels(camera, one_label, measure, &calls, out);
        result = out[0];
        return n;
    };
    PlacedLabel wrapped_label;
    std::array<LabelRun, kMaxLabelLines> runs;
    CHECK(run_one(wrapping(64, 64, "aaaa bbbb cccc", LabelRole::area, LabelSide::center, 45),
                  wrapped_label) == 1);
    CHECK(wrapped_label.box == (LabelBox{42, 56, 87, 72}));
    CHECK(label_runs(wrapped_label, "aaaa bbbb cccc", runs) == 2);
    CHECK(runs[0].text == "aaaa bbbb" && runs[0].x == 44 && runs[0].y == 62);
    CHECK(runs[1].text == "cccc" && runs[1].x == 56 && runs[1].y == 70);
    CHECK(run_one(wrapping(64, 64, "aa bb", LabelRole::point, LabelSide::right, 9),
                  wrapped_label) == 1);
    CHECK(label_runs(wrapped_label, "aa bb", runs) == 2);
    CHECK(runs[0].text == "aa" && runs[1].text == "bb" && runs[0].x == 70 && runs[1].x == 70);
    CHECK(run_one(wrapping(64, 64, "a b c d e f", LabelRole::point, LabelSide::center, 5),
                  wrapped_label) == 1);
    CHECK(label_runs(wrapped_label, "a b c d e f", runs) == 3);
    CHECK(runs[0].text == "a" && runs[1].text == "b" && runs[2].text == "c d e f");
    CHECK(run_one(wrapping(64, 64, "longword", LabelRole::area, LabelSide::center, 10),
                  wrapped_label) == 1);
    CHECK(label_runs(wrapped_label, "longword", runs) == 1 && runs[0].text == "longword");
    CHECK(run_one(wrapping(64, 64, "aaaa bbbb", LabelRole::area, LabelSide::center, 0),
                  wrapped_label) == 1 &&
          wrapped_label.line_count == 0);
    return TEST_MAIN_RESULT();
}
