// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/label_layout.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace globe {
namespace {

bool overlaps(LabelBox a, LabelBox b) {
    return a.left < b.right && b.left < a.right && a.top < b.bottom && b.top < a.bottom;
}

bool fits(const Camera& camera, LabelBox box) {
    if (box.left < 0 || box.top < 0 || box.right > camera.width || box.bottom > camera.height) {
        return false;
    }
    // The farthest corner combines the farthest x and y edges. Normalize before squaring.
    const double dx =
        std::max(std::abs(box.left - camera.center_x), std::abs(box.right - camera.center_x)) /
        camera.radius_px;
    const double dy =
        std::max(std::abs(box.top - camera.center_y), std::abs(box.bottom - camera.center_y)) /
        camera.radius_px;
    return dx * dx + dy * dy <= 1.0;
}

bool valid_metrics(LabelMetrics metrics) {
    constexpr int limit = kMaxViewportDimension;
    return metrics.left >= -limit && metrics.right <= limit && metrics.left < metrics.right &&
           metrics.ascent >= 0 && metrics.ascent <= limit && metrics.descent >= 0 &&
           metrics.descent <= limit && metrics.ascent + metrics.descent > 0;
}

// The text of a label cut into lines, each measured.
struct Wrapped {
    std::size_t count = 0;
    std::array<LabelLine, kMaxLabelLines> lines{};
    std::array<LabelMetrics, kMaxLabelLines> metrics{};
};

// Greedy wrap at spaces: a line takes words while it is at most max_width wide; a single word
// wider than that stays whole; the last line takes whatever is left. nullopt if the provider
// cannot measure a run or a measure is invalid.
std::optional<Wrapped> wrap(std::string_view text, int max_width, LabelRole role,
                            MeasureLabel measure, void* context) {
    Wrapped out;
    const auto measure_run = [&](std::size_t begin, std::size_t end) {
        const auto m = measure(context, text.substr(begin, end - begin), role);
        return m && valid_metrics(*m) ? m : std::nullopt;
    };
    std::size_t begin = 0;
    while (out.count < kMaxLabelLines) {
        std::size_t end = text.size();
        auto metrics = measure_run(begin, end);
        if (!metrics)
            return std::nullopt;
        // Not the last allowed line and too wide: pull the break back to the last space that fits.
        if (max_width > 0 && out.count + 1 < kMaxLabelLines &&
            metrics->right - metrics->left > max_width) {
            std::size_t space = begin;
            std::optional<LabelMetrics> fit;
            std::size_t fit_end = 0;
            while ((space = text.find(' ', space + 1)) != std::string_view::npos) {
                const auto m = measure_run(begin, space);
                if (!m)
                    return std::nullopt;
                if (m->right - m->left > max_width && fit)
                    break;
                fit = m; // the first word is kept even if it alone is too wide
                fit_end = space;
                if (m->right - m->left > max_width)
                    break;
            }
            if (fit) {
                end = fit_end;
                metrics = fit;
            }
        }
        out.lines[out.count] = {static_cast<std::uint8_t>(begin),
                                static_cast<std::uint8_t>(end - begin), 0};
        out.metrics[out.count++] = *metrics;
        if (end >= text.size())
            break;
        begin = end + 1; // the space
        while (begin < text.size() && text[begin] == ' ')
            ++begin;
        if (begin >= text.size())
            break;
    }
    return out;
}

} // namespace

std::size_t label_runs(const PlacedLabel& placed, std::string_view text,
                       std::array<LabelRun, kMaxLabelLines>& out) {
    if (placed.line_count == 0) {
        out[0] = {text, placed.baseline_x, placed.baseline_y};
        return 1;
    }
    for (std::size_t i = 0; i < placed.line_count; ++i) {
        const auto& line = placed.lines[i];
        out[i] = {text.substr(line.begin, line.length), line.x,
                  placed.baseline_y + static_cast<int>(i) * placed.line_pitch};
    }
    return placed.line_count;
}

std::size_t place_labels(const Camera& camera, std::span<const LabelCandidate> candidates,
                         MeasureLabel measure, void* context, std::span<PlacedLabel> output,
                         std::span<const PlacedLabel> avoid) {
    const auto cap = std::min(output.size(), kMaxPlacedLabels);
    if (cap == 0 || !measure || candidates.size() > kMaxLabelCandidates) {
        return 0;
    }

    std::array<std::uint16_t, kMaxLabelCandidates> order;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        order[i] = static_cast<std::uint16_t>(i);
    }
    const auto end = order.begin() + static_cast<std::ptrdiff_t>(candidates.size());
    std::sort(order.begin(), end, [candidates](auto a, auto b) {
        const int pa = candidates[a].priority;
        const int pb = candidates[b].priority;
        return pa == pb ? a < b : pa > pb;
    });

    std::size_t count = 0;
    for (auto index = order.begin(); index != end && count < cap; ++index) {
        const auto& label = candidates[*index];
        const Vec3 anchor = to_camera_space(camera, label.anchor);
        const ScreenPoint point = to_screen(camera, anchor);
        if (!is_front(anchor) || label.text.empty() ||
            !(point.x >= 0 && point.x < camera.width && point.y >= 0 && point.y < camera.height)) {
            continue;
        }

        const auto wrapped = wrap(label.text, label.max_width, label.role, measure, context);
        if (!wrapped) {
            continue;
        }
        // One box for all lines: the widest line wide, every line one pitch high.
        int width = 0, ascent = 0, descent = 0;
        for (std::size_t i = 0; i < wrapped->count; ++i) {
            const auto& m = wrapped->metrics[i];
            width = std::max(width, m.right - m.left);
            ascent = std::max(ascent, m.ascent);
            descent = std::max(descent, m.descent);
        }
        const int pitch = ascent + descent;
        const int height = pitch * static_cast<int>(wrapped->count);
        const bool beside = label.side == LabelSide::right;
        const int left =
            beside ? static_cast<int>(point.x) + kMarkerGap : static_cast<int>(point.x) - width / 2;
        const int top = static_cast<int>(point.y) - height / 2;
        // Only a label beside its marker keeps clear of neighbours; centred labels may touch.
        const int gap = beside ? kNeighbourGap : 0;
        const auto box_at = [&](int dx, int dy) {
            return LabelBox{left + dx, top + dy, left + dx + width, top + dy + height};
        };
        const auto hits = [gap](LabelBox box) {
            const LabelBox padded{box.left - gap, box.top - gap, box.right + gap, box.bottom + gap};
            return [padded](const PlacedLabel& placed) { return overlaps(padded, placed.box); };
        };
        const auto taken = [&](LabelBox box) {
            return std::any_of(output.begin(), output.begin() + static_cast<std::ptrdiff_t>(count),
                               hits(box));
        };
        const auto avoided = [&](LabelBox box) {
            return std::any_of(avoid.begin(), avoid.end(), hits(box));
        };
        LabelBox box = box_at(0, 0);
        if (!fits(camera, box) || taken(box))
            continue;
        int dx = 0, dy = 0;
        if (avoided(box)) {
            if (beside)
                continue; // a label beside its marker is dropped, never moved
            // A centred label ducks: up, down, right, then left of where it was, until clear. With
            // nowhere clear it stays put and is drawn over what it avoids.
            const int up = height / 2 + kDuckMargin, across = width / 2 + kDuckMargin;
            const std::array<std::array<int, 2>, 4> moves{
                {{0, -up}, {0, up}, {across, 0}, {-across, 0}}};
            for (const auto& move : moves) {
                const auto moved = box_at(move[0], move[1]);
                if (fits(camera, moved) && !taken(moved) && !avoided(moved)) {
                    dx = move[0];
                    dy = move[1];
                    box = moved;
                    break;
                }
            }
        }
        PlacedLabel placed{*index, box.left - wrapped->metrics[0].left, box.top + ascent, box};
        if (wrapped->count > 1) {
            placed.line_count = static_cast<std::uint8_t>(wrapped->count);
            placed.line_pitch = static_cast<std::int16_t>(pitch);
            for (std::size_t i = 0; i < wrapped->count; ++i) {
                const auto& m = wrapped->metrics[i];
                // Centred labels centre each line in the box; labels beside a dot align left.
                const int line_width = m.right - m.left;
                const int line_left = beside ? box.left : box.left + (width - line_width) / 2;
                placed.lines[i] = wrapped->lines[i];
                placed.lines[i].x = static_cast<std::int16_t>(line_left - m.left);
            }
            placed.baseline_x = placed.lines[0].x;
        } else {
            // A single line keeps the box's own left edge, as an unwrapped label always has.
            placed.baseline_x = box.left - wrapped->metrics[0].left;
        }
        output[count++] = placed;
    }
    return count;
}

} // namespace globe
