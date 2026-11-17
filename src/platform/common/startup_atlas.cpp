// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "startup_atlas.hpp"
#include <new>
#include <utility>

namespace globe {

std::unique_ptr<std::uint8_t[]> allocate_atlas_bytes(std::size_t size) {
    return std::unique_ptr<std::uint8_t[]>(new (std::nothrow) std::uint8_t[size]);
}

std::optional<Scene> make_atlas_scene(const Atlas& atlas) {
    return Scene::create(atlas);
}

StartupAtlas read_startup_atlas(long size, void* file, AtlasRead read, AtlasAllocate allocate,
                                AtlasMakeScene make_scene) {
    if (size <= 0)
        return {{}, {}, "Empty atlas"};
    if (static_cast<unsigned long>(size) > kStartupAtlasMaxBytes)
        return {{}, {}, "Atlas exceeds 64 KiB"};
    auto bytes = allocate(static_cast<std::size_t>(size));
    if (!bytes)
        return {{}, {}, "Atlas allocation failed"};
    if (!read(file, {bytes.get(), static_cast<std::size_t>(size)}))
        return {{}, {}, "Atlas read failed"};
    AtlasError error;
    auto atlas = Atlas::parse(bytes.get(), static_cast<std::size_t>(size), &error);
    if (!atlas)
        return {{}, {}, atlas_error_name(error)};
    auto scene = make_scene(*atlas);
    if (!scene)
        return {{}, {}, "Atlas scene allocation failed"};
    return {std::move(bytes), std::move(scene), nullptr};
}

StartupAtlas with_embedded_fallback(StartupAtlas candidate,
                                    std::span<const std::uint8_t> embedded) {
    if (!candidate.scene) {
        candidate.bytes.reset();
        if (auto atlas = Atlas::parse(embedded.data(), embedded.size()))
            candidate.scene = Scene::create(*atlas);
    }
    return candidate;
}

} // namespace globe
