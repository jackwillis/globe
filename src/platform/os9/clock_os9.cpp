// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "clock.hpp"
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
