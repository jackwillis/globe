// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// The platform's monotonic clock. Core code never calls it: time enters the application only
// inside messages. Each platform provides one definition (clock_posix.cpp for Linux and NetBSD;
// the Mac OS 9 build will supply its own, for example from Microseconds()).
#pragma once

#include <cstdint>

namespace globe {

// Nanoseconds from an arbitrary fixed origin; never goes backwards.
std::uint64_t monotonic_ns();

} // namespace globe
