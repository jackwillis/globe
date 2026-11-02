// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "elevation_packs.hpp"
#include "globe/bytes.hpp"
#include "globe/crc32.hpp"
#include <cstdio>
#include <string>
#include <vector>
using namespace globe;
int main(int argc, char** argv) {
    const std::string root = argc > 1 ? argv[1] : ".";
    const std::string temp =
        (argc > 2 ? std::string(argv[2]) : ".") + "/elevation-import-test.gelv";
    std::vector<std::uint8_t> bytes;
    CHECK(read_pack_file((root + "/data/elevation-1deg.gelv").c_str(), bytes));
    ElevationPacks packs;
    CHECK(packs.initialize(bytes.data(), bytes.size()));
    CHECK(!packs.import_file("/no-such-globe-pack.gelv"));
    CHECK(!packs.import_file((root + "/tests/data/elevation.gelv").c_str()));
    auto fine = bytes;
    fine.resize(552 + 720 * 360, 128);
    store_le32(fine.data() + 8, 720);
    store_le32(fine.data() + 12, 360);
    store_le32(fine.data() + 36,
               crc32_extend(crc32(fine.data(), 36), fine.data() + 40, fine.size() - 40));
    auto* file = std::fopen(temp.c_str(), "wb");
    CHECK(file);
    if (file) {
        CHECK(std::fwrite(fine.data(), 1, fine.size(), file) == fine.size());
        std::fclose(file);
    }
    auto corrupt = fine;
    corrupt.back() ^= 1;
    file = std::fopen(temp.c_str(), "wb");
    CHECK(file);
    if (file) {
        CHECK(std::fwrite(corrupt.data(), 1, corrupt.size(), file) == corrupt.size());
        std::fclose(file);
    }
    CHECK(!packs.import_file(temp.c_str()));
    file = std::fopen(temp.c_str(), "wb");
    CHECK(file);
    if (file) {
        CHECK(std::fwrite(fine.data(), 1, fine.size(), file) == fine.size());
        std::fclose(file);
    }
    CHECK(packs.import_file(temp.c_str()));
    Model model = initial_model(640, 480);
    packs.register_levels(model);
    model.stage = QualityStage::Refine;
    model.camera.radius_px = 400;
    CHECK(packs.for_model(model) && packs.for_model(model)->grid().width() == 720);
    model.camera.radius_px = 180;
    CHECK(packs.for_model(model) && packs.for_model(model)->grid().width() == 360);
    CHECK(packs.import_file(temp.c_str()));  // two fine caches still fit the aggregate budget
    CHECK(!packs.import_file(temp.c_str())); // a third would exceed it
    CHECK(packs.bytes() <= kTerrainCacheBudget && packs.count() == 3);
    file = std::fopen(temp.c_str(), "wb");
    CHECK(file);
    if (file) {
        std::fseek(file, static_cast<long>(kElevationMaxCells + 552), SEEK_SET);
        std::fputc(0, file);
        std::fclose(file);
    }
    CHECK(!read_pack_file(temp.c_str(), fine)); // reject oversized input before allocating its size
    std::remove(temp.c_str());
    return TEST_MAIN_RESULT();
}
