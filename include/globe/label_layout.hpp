// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once

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

struct PlacedLabel {
    std::size_t candidate;
    int baseline_x, baseline_y;
    LabelBox box;
    bool operator==(const PlacedLabel&) const = default;
};

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
