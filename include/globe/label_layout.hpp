// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once

#include <optional>
#include <span>
#include <string_view>

#include "globe/projection.hpp"

namespace globe {

enum class LabelRole { area, point };

struct LabelCandidate {
    Vec3 anchor; // Earth-centered unit vector.
    std::string_view text;
    LabelRole role;
    int priority; // Higher first, ties in input order.
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
std::size_t place_labels(const Camera& camera, std::span<const LabelCandidate> candidates,
                         MeasureLabel measure, void* context, std::span<PlacedLabel> output);

} // namespace globe
