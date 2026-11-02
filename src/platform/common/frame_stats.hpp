// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Distribution summary for timing samples. Portable: no OS calls. The monotonic clock that
// produces the samples is separate (clock.hpp), because each platform reads time differently.
#pragma once

#include <cstddef>

namespace globe {

struct Summary {
    std::size_t count = 0;
    double min = 0.0;
    double mean = 0.0;
    double p50 = 0.0; // nearest-rank percentiles
    double p95 = 0.0;
    double max = 0.0;
};

// Summarizes samples (finite values). Sorts `samples` in place. Zero samples give a zeroed
// Summary. Nearest-rank percentile: the value at index ceil(p * n) - 1 of the sorted samples.
Summary summarize(double* samples, std::size_t count);

} // namespace globe
