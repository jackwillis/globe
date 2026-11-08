// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Standalone host-only feasibility probe. No runtime/build integration.
#include "globe/elevation.hpp"
#include "globe/terrain.hpp"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        std::ifstream file(argv[i], std::ios::binary);
        if (!file)
            return 1;
        const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file),
                                              std::istreambuf_iterator<char>()};
        const auto grid = globe::Elevation::parse(bytes.data(), bytes.size());
        if (!grid)
            return 2;
        const auto needed = globe::TerrainScene::required_bytes(*grid);
        const auto scene = globe::TerrainScene::create(*grid);
        std::printf("{\"width\":%u,\"height\":%u,\"packed_bytes\":%zu,"
                    "\"terrain_required_bytes\":%zu,\"terrain_budget_bytes\":%zu,"
                    "\"terrain_created\":%s}\n",
                    grid->width(), grid->height(), bytes.size(), needed, globe::kTerrainCacheBudget,
                    scene ? "true" : "false");
    }
}
