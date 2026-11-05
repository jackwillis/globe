// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "wall_clock.hpp"
using namespace globe;
int main() {
    constexpr std::uint32_t kMacEpochToUnix = 2082844800U;
    // 2026-11-04 12:00:00 UTC = UNIX 1793793600. UTC clock, no offset.
    CHECK(unix_from_mac(1793793600U + kMacEpochToUnix, 0) == 1793793600);
    // US Central standard time (UTC-6): the Mac clock reads 06:00 when UTC is 12:00.
    const std::uint32_t central = static_cast<std::uint32_t>(-6 * 3600) & 0x00FFFFFFU;
    CHECK(unix_from_mac(1793793600U - 6 * 3600 + kMacEpochToUnix, central) == 1793793600);
    // Central daylight time: the clock reads 07:00 and the dlsDelta bit is set.
    CHECK(unix_from_mac(1793793600U - 5 * 3600 + kMacEpochToUnix, central | 0x80000000U) ==
          1793793600);
    // East of Greenwich (UTC+9, Japan): the clock reads 21:00.
    CHECK(unix_from_mac(1793793600U + 9 * 3600 + kMacEpochToUnix, 9 * 3600) == 1793793600);
    // The date before the UNIX epoch is representable (the offset math is signed).
    CHECK(unix_from_mac(kMacEpochToUnix - 100, 0) == -100);
    // The real clock is positive and recent.
    CHECK(unix_seconds() > 1790000000);
    return TEST_MAIN_RESULT();
}
