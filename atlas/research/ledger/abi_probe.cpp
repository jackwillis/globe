// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Compile-only ABI constants; reading assembly is not a target runtime/heap measurement.
#include "globe/framebuffer.hpp"
#include "globe/memory_ledger.hpp"
#include <cstdint>
extern "C" {
std::uint32_t ledger_size = sizeof(globe::MemoryLedger);
std::uint32_t charge_size = sizeof(globe::MemoryCharge);
std::uint32_t framebuffer_size = sizeof(globe::Framebuffer);
std::uint32_t pointer_size = sizeof(void*);
}
