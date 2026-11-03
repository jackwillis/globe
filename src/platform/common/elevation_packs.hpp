// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include "globe/app.hpp"
#include "globe/terrain.hpp"
#include <array>
#include <vector>
namespace globe {
// Startup file reads only. Reject nonseekable/oversized inputs before resizing the output.
bool read_pack_file(const char* path, std::vector<std::uint8_t>& out);
class ElevationPacks {
  public:
    bool initialize(const std::uint8_t* bytes, std::size_t size);
    bool import_file(const char* path);
    void register_levels(Model& model) const;
    TerrainScene* for_model(const Model& model);
    std::size_t bytes() const { return bytes_; }
    unsigned count() const { return count_; }
    const char* error() const { return error_; }

  private:
    bool add(const std::uint8_t* bytes, std::size_t size);
    std::array<std::vector<std::uint8_t>, kElevationLevels> files_;
    std::array<std::optional<TerrainScene>, kElevationLevels> scenes_;
    std::optional<LandClasses> classes_;
    unsigned count_ = 0;
    std::size_t bytes_ = 0;
    const char* error_ = "";
};
} // namespace globe
