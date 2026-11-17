// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once

#include "globe/render.hpp"
#include <memory>
#include <span>

namespace globe {

// A startup I/O budget, independent of the GATL format limits. Bounds the
// derived Scene too: each vertex costs four file bytes, each line table entry four.
inline constexpr std::size_t kStartupAtlasMaxBytes = 64 * 1024;

struct StartupAtlas {
    // Reverse destruction order keeps the zero-copy view valid.
    std::unique_ptr<const std::uint8_t[]> bytes;
    std::optional<Scene> scene;
    const char* diagnostic = nullptr;
};

using AtlasRead = bool (*)(void*, std::span<std::uint8_t>);
using AtlasAllocate = std::unique_ptr<std::uint8_t[]> (*)(std::size_t);
using AtlasMakeScene = std::optional<Scene> (*)(const Atlas&);

std::unique_ptr<std::uint8_t[]> allocate_atlas_bytes(std::size_t size);
std::optional<Scene> make_atlas_scene(const Atlas& atlas);
StartupAtlas read_startup_atlas(long size, void* file, AtlasRead read,
                                AtlasAllocate allocate = allocate_atlas_bytes,
                                AtlasMakeScene make_scene = make_atlas_scene);

StartupAtlas with_embedded_fallback(StartupAtlas candidate, std::span<const std::uint8_t> embedded);

} // namespace globe
