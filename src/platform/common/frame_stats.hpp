// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Distribution summary for timing samples, plus the monotonic clock the platform layer reads.
// Core code never calls the clock: time enters the application only inside messages.
#pragma once

#include <cstddef>
#include <cstdint>

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

// Nanoseconds from an arbitrary fixed origin; never goes backwards (CLOCK_MONOTONIC).
std::uint64_t monotonic_ns();

} // namespace globe
