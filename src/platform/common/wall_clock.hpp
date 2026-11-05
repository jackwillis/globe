// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Wall-clock time for display features (the day/night terminator). Never used by the core: time
// reaches the application only inside messages, and this is read by the front end at display time.
#pragma once
#include <cstdint>
namespace globe {
// UNIX time (seconds since 1970-01-01 UTC) now.
std::int64_t unix_seconds();
// Classic Mac OS keeps local time as seconds since 1904-01-01. `gmt_field` is the gmtDelta long of
// the Map control panel's MachineLocation: the low 24 bits are the signed offset east of GMT in
// seconds, and bit 7 of the high byte (dlsDelta) means daylight saving time is in effect (one
// hour more). UTC = local - offset. This is pure so it can be checked off the Mac.
constexpr std::int64_t unix_from_mac(std::uint32_t mac_seconds, std::uint32_t gmt_field) {
    std::int64_t offset = static_cast<std::int64_t>(gmt_field & 0x00FFFFFFU);
    if ((offset & 0x00800000) != 0)
        offset -= 0x01000000; // sign-extend the 24-bit value
    if ((gmt_field & 0x80000000U) != 0)
        offset += 3600;
    return static_cast<std::int64_t>(mac_seconds) - 2082844800LL - offset;
}
} // namespace globe
