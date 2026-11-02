// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// The bundled atlas compiled into the program (generated from data/atlas.gatl at build time by
// tools/embed_atlas.py). Link the globe_embedded_atlas target to use it.
#pragma once

#include <cstddef>
#include <cstdint>

namespace globe {

extern const std::uint8_t kEmbeddedAtlasData[];
extern const std::size_t kEmbeddedAtlasSize;

} // namespace globe
