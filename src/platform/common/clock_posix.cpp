// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include <ctime>

#include "clock.hpp"
#include "wall_clock.hpp"

namespace globe {

std::uint64_t monotonic_ns() {
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<std::uint64_t>(ts.tv_sec) * 1000000000ULL +
           static_cast<std::uint64_t>(ts.tv_nsec);
}

std::int64_t unix_seconds() {
    return static_cast<std::int64_t>(std::time(nullptr));
}

} // namespace globe
