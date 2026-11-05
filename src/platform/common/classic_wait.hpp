// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include <cstdint>
namespace globe {
// WaitNextEvent uses 60-Hz ticks. Round future deadlines up; no recurring idle timer.
constexpr std::uint32_t classic_wait_ticks(std::uint64_t now_ms, std::uint64_t wake_at_ms,
                                           bool work, bool foreground = true) {
    constexpr std::uint32_t forever = 0x7fffffffU;
    if (!foreground)
        return forever; // suspend all application work and deadlines until a resume event
    if (work)
        return 0;
    if (!wake_at_ms)
        return forever;
    if (wake_at_ms <= now_ms)
        return 0;
    const auto delta = wake_at_ms - now_ms;
    if (delta >= static_cast<std::uint64_t>(forever) * 1000 / 60)
        return forever;
    return static_cast<std::uint32_t>((delta * 60 + 999) / 1000);
}
} // namespace globe
