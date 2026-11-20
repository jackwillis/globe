// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

#include "globe/projection.hpp"

namespace globe {

enum class LabelRole : std::uint8_t { area, point };
// Where the box sits against the anchor: centred on it, or to its right with a gap so a point
// marker stays visible.
enum class LabelSide : std::uint8_t { center, right };

struct LabelCandidate {
    Vec3 anchor; // Earth-centered unit vector.
    std::string_view text;
    LabelRole role;
    int priority; // Higher first, ties in input order.
    LabelSide side = LabelSide::center;
    // Wider than this many pixels wraps at spaces into up to kMaxLabelLines lines. 0 means never.
    int max_width = 0;
};

// Conservative painted bounds in pixels, relative to the baseline origin.
// Horizontal bearings may be negative. Ascent and descent are nonnegative.
struct LabelMetrics {
    int left, right, ascent, descent;
};

using MeasureLabel = std::optional<LabelMetrics> (*)(void*, std::string_view, LabelRole);

struct LabelBox {
    int left, top, right, bottom; // Half-open pixel rectangle.
    bool operator==(const LabelBox&) const = default;
};

constexpr std::size_t kMaxLabelLines = 3;
// Default wrap widths in pixels for country and city names (owner, 2026-11-19; first guesses).
constexpr int kCountryLabelWidth = 80;
constexpr int kCityLabelWidth = 64;

// One line of a wrapped label: a slice of the candidate text and the baseline x it is drawn at.
struct LabelLine {
    std::uint8_t begin = 0, length = 0;
    std::int16_t x = 0;
    bool operator==(const LabelLine&) const = default;
};

struct PlacedLabel {
    std::size_t candidate;
    int baseline_x, baseline_y; // First (or only) line.
    LabelBox box;
    // line_count 0 is a single line of the whole text at baseline_x. Otherwise `lines` hold each
    // line's slice and baseline x, and baseline y advances by line_pitch.
    std::uint8_t line_count = 0;
    std::int16_t line_pitch = 0;
    std::array<LabelLine, kMaxLabelLines> lines{};
    bool operator==(const PlacedLabel&) const = default;
};

// Where to draw one run of text.
struct LabelRun {
    std::string_view text;
    int x, y;
};
// Expands a placed label into the runs to draw, for its candidate's text: one for a single line,
// up to kMaxLabelLines for a wrapped one. Returns how many were written.
std::size_t label_runs(const PlacedLabel& placed, std::string_view text,
                       std::array<LabelRun, kMaxLabelLines>& out);

constexpr std::size_t kMaxLabelCandidates = 256;
constexpr std::size_t kMaxPlacedLabels = 32;

// Uses the shown frame camera. Caller preselects at most kMaxLabelCandidates; oversized
// batches return zero. Output capacity is the requested cap, clamped to kMaxPlacedLabels.
// A provider returns nullopt for an unavailable run. No text is retained; output indexes
// the input batch. Input and output storage must not overlap.
//
// `avoid` holds boxes the caller wants kept clear: labels placed by an earlier call, or marker
// dots. A label beside its anchor is dropped if it would overlap one, and keeps kNeighbourGap
// pixels from every other box. A centred label moves up, down, right or left (by half its size
// plus kDuckMargin) to the first spot that is clear of `avoid`, and stays where it was if none is.
// Overlap with labels placed in this call still drops a label, as before.
constexpr int kNeighbourGap = 2;
constexpr int kMarkerGap = 4;
constexpr int kDuckMargin = 4;
std::size_t place_labels(const Camera& camera, std::span<const LabelCandidate> candidates,
                         MeasureLabel measure, void* context, std::span<PlacedLabel> output,
                         std::span<const PlacedLabel> avoid = {});

} // namespace globe
