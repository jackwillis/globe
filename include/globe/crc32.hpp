// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// CRC-32 (IEEE 802.3, reflected, polynomial 0xEDB88320), the same function as zlib's crc32.
#pragma once

#include <cstddef>
#include <cstdint>

namespace globe {

std::uint32_t crc32_extend(std::uint32_t previous, const std::uint8_t* data, std::size_t size);

std::uint32_t crc32(const std::uint8_t* data, std::size_t size);

} // namespace globe
