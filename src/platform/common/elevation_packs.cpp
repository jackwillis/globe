// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "elevation_packs.hpp"
#include <cstdio>
namespace globe {
bool read_pack_file(const char* path, std::vector<std::uint8_t>& out) {
    out.clear();
    if (!path)
        return false;
    auto* file = std::fopen(path, "rb");
    if (!file)
        return false;
    if (std::fseek(file, 0, SEEK_END) != 0) {
        std::fclose(file);
        return false;
    }
    const long length = std::ftell(file);
    if (length < static_cast<long>(kElevationCellsOffset) ||
        static_cast<unsigned long>(length) > kElevationCellsOffset + kElevationMaxCells ||
        std::fseek(file, 0, SEEK_SET) != 0) {
        std::fclose(file);
        return false;
    }
    out.resize(static_cast<std::size_t>(length));
    const bool ok = std::fread(out.data(), 1, out.size(), file) == out.size() &&
                    std::fgetc(file) == EOF && std::ferror(file) == 0;
    const int closed = std::fclose(file);
    if (!ok || closed != 0) {
        out.clear();
        return false;
    }
    return true;
}
bool ElevationPacks::initialize(const std::uint8_t* data, std::size_t size) {
    for (auto& scene : scenes_)
        scene.reset();
    count_ = 0;
    bytes_ = 0;
    error_ = "";
    return add(data, size);
}
bool ElevationPacks::add(const std::uint8_t* data, std::size_t size) {
    if (count_ >= kElevationLevels) {
        error_ = "Too many elevation packs";
        return false;
    }
    const auto grid = Elevation::parse(data, size);
    if (!grid) {
        error_ = "Invalid elevation pack";
        return false;
    }
    if (grid->west_deg() != -180 || grid->east_deg() != 180 || grid->south_deg() != -90 ||
        grid->north_deg() != 90 || grid->width() != 2 * grid->height()) {
        error_ = "Elevation packs must cover the whole globe with equal angular cell sizes";
        return false;
    }
    if (TerrainScene::required_bytes(*grid) > kTerrainCacheBudget - bytes_) {
        error_ = "Elevation packs exceed the terrain memory budget";
        return false;
    }
    auto scene = TerrainScene::create(*grid);
    if (!scene) {
        error_ = "Not enough memory for elevation";
        return false;
    }
    bytes_ += scene->bytes();
    scenes_[count_++] = std::move(scene);
    error_ = "";
    return true;
}
bool ElevationPacks::import_file(const char* path) {
    if (count_ >= kElevationLevels) {
        error_ = "Too many elevation packs";
        return false;
    }
    auto& data = files_[count_];
    if (!read_pack_file(path, data)) {
        error_ = "Cannot read elevation pack (missing, nonseekable or too large)";
        return false;
    }
    if (!add(data.data(), data.size())) {
        std::vector<std::uint8_t>().swap(data);
        return false;
    }
    return true;
}
void ElevationPacks::register_levels(Model& model) const {
    for (unsigned i = 0; i < count_; ++i) {
        const auto& grid = scenes_[i]->grid();
        update(model, RegisterElevation{i, grid.width(), grid.height()});
    }
}
TerrainScene* ElevationPacks::for_model(const Model& model) {
    const auto level = quality_for(model).elevation_level;
    if (level < count_ && scenes_[level])
        return &*scenes_[level];
    return count_ && scenes_[0] ? &*scenes_[0] : nullptr;
}
} // namespace globe
