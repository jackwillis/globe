// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "frame_stats.hpp"

#include <algorithm>
#include <cmath>

namespace globe {

namespace {

double nearest_rank(const double* sorted, std::size_t n, double p) {
    const double rank = std::ceil(p * static_cast<double>(n));
    const std::size_t index = rank < 1.0 ? 0 : static_cast<std::size_t>(rank) - 1;
    return sorted[std::min(index, n - 1)];
}

} // namespace

Summary summarize(double* samples, std::size_t count) {
    Summary s;
    if (samples == nullptr || count == 0) {
        return s;
    }
    std::sort(samples, samples + count);
    double sum = 0.0;
    for (std::size_t i = 0; i < count; ++i) {
        sum += samples[i];
    }
    s.count = count;
    s.min = samples[0];
    s.max = samples[count - 1];
    s.mean = sum / static_cast<double>(count);
    s.p50 = nearest_rank(samples, count, 0.50);
    s.p95 = nearest_rank(samples, count, 0.95);
    return s;
}

} // namespace globe
