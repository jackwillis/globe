// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "clock.hpp"
#include "wall_clock.hpp"
// Multiversal omits this declaration, but InterfaceLib exports the OS 9 function.
// This is our minimal ABI declaration; no system header or implementation is copied.
struct GlobeClockWide {
    std::uint32_t hi, lo;
};
extern "C" void Microseconds(GlobeClockWide* value);
namespace globe {
std::uint64_t monotonic_ns() {
    GlobeClockWide value{};
    Microseconds(&value);
    return ((static_cast<std::uint64_t>(value.hi) << 32) | value.lo) * 1000;
}
} // namespace globe

// Wall clock for the terminator. Multiversal omits these declarations, but InterfaceLib exports the
// OS 9 functions; these are minimal ABI declarations (no system header or implementation copied).
struct GlobeMachineLocation {
    std::int32_t latitude, longitude;
    std::int32_t gmt_field; // union { char dlsDelta; long gmtDelta; }
};
extern "C" void ReadLocation(GlobeMachineLocation* location);
extern "C" void GetDateTime(std::uint32_t* seconds);
namespace globe {
std::int64_t unix_seconds() {
    std::uint32_t mac = 0;
    GetDateTime(&mac);
    GlobeMachineLocation location{};
    ReadLocation(&location); // the Map control panel; zero (UTC) if it was never set
    return unix_from_mac(mac, static_cast<std::uint32_t>(location.gmt_field));
}
} // namespace globe
