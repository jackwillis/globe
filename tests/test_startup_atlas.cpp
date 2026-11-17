// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/refine_runner.hpp"
#include "startup_atlas.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string_view>
#include <utility>
#include <vector>

using namespace globe;
namespace {

using Bytes = std::vector<std::uint8_t>;

Bytes file_bytes(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), {}};
}

bool read_fixture(void* file, std::span<std::uint8_t> out) {
    const auto& bytes = *static_cast<const Bytes*>(file);
    if (bytes.size() != out.size())
        return false;
    std::copy(bytes.begin(), bytes.end(), out.begin());
    return true;
}

StartupAtlas load(Bytes& bytes) {
    return read_startup_atlas(static_cast<long>(bytes.size()), &bytes, read_fixture);
}

void check_fallback(StartupAtlas failed, const Bytes& embedded, std::string_view diagnostic) {
    CHECK(!failed.bytes && !failed.scene);
    CHECK(failed.diagnostic && diagnostic == failed.diagnostic);
    const auto baseline = Atlas::parse(embedded.data(), embedded.size());
    const auto restored = with_embedded_fallback(std::move(failed), embedded);
    CHECK(!restored.bytes && restored.scene && baseline);
    if (restored.scene && baseline)
        CHECK(restored.scene->atlas().point_count() == baseline->point_count());
}

} // namespace

int main(int argc, char** argv) {
    const std::filesystem::path root = argc > 1 ? argv[1] : ".";
    auto embedded = file_bytes(root / "data/atlas.gatl");
    auto tiny = file_bytes(root / "tests/data/tiny.gatl");
    CHECK(embedded.size() >= 32 && !tiny.empty());
    if (embedded.size() < 32 || tiny.empty())
        return 1;

    auto parity = with_embedded_fallback(load(embedded), embedded);
    CHECK(parity.scene && parity.bytes && !parity.diagnostic);
    auto distinct = with_embedded_fallback(load(tiny), embedded);
    CHECK(distinct.scene && distinct.bytes && !distinct.diagnostic);
    if (distinct.scene && parity.scene) {
        CHECK(distinct.scene->atlas().point_count() != parity.scene->atlas().point_count());
        std::fill(tiny.begin(), tiny.end(), 0); // the scene reads the owned copy
        auto frame = Framebuffer::create(160, 120);
        auto baseline_frame = Framebuffer::create(160, 120);
        auto baseline = with_embedded_fallback({}, embedded);
        CHECK(frame && baseline_frame && baseline.scene);
        if (frame && baseline_frame && baseline.scene) {
            auto model = initial_model(160, 120);
            RefineRunner embedded_runner(*baseline.scene);
            RefineRunner parity_runner(*parity.scene);
            embedded_runner.render_moving(*baseline_frame, model, nullptr);
            parity_runner.render_moving(*frame, model, nullptr);
            const auto count = frame->width() * frame->height();
            CHECK(std::equal(frame->data(), frame->data() + count, baseline_frame->data()));

            RefineRunner runner(*distinct.scene);
            runner.render_moving(*frame, model, nullptr);
            CHECK(!std::equal(frame->data(), frame->data() + count, baseline_frame->data()));
            runner.start(model, &*frame, nullptr);
            CHECK(runner.active());
            for (unsigned i = 0; i < 1000 && runner.active(); ++i)
                runner.step(*frame, model);
            CHECK(!runner.active());
        }
    }

    check_fallback({{}, {}, "Data folder unavailable"}, embedded, "Data folder unavailable");
    check_fallback({{}, {}, "Data/atlas.gatl unavailable"}, embedded,
                   "Data/atlas.gatl unavailable");
    check_fallback(read_startup_atlas(0, nullptr, nullptr), embedded, "Empty atlas");
    check_fallback(read_startup_atlas(kStartupAtlasMaxBytes + 1, nullptr, nullptr), embedded,
                   "Atlas exceeds 64 KiB");
    check_fallback(read_startup_atlas(32, &embedded, read_fixture), embedded, "Atlas read failed");
    check_fallback(
        read_startup_atlas(32, nullptr, nullptr,
                           [](std::size_t) { return std::unique_ptr<std::uint8_t[]>{}; }),
        embedded, "Atlas allocation failed");
    check_fallback(read_startup_atlas(static_cast<long>(embedded.size()), &embedded, read_fixture,
                                      allocate_atlas_bytes,
                                      [](const Atlas&) { return std::optional<Scene>{}; }),
                   embedded, "Atlas scene allocation failed");

    Bytes invalid = embedded;
    invalid.pop_back();
    check_fallback(load(invalid), embedded, atlas_error_name(AtlasError::SizeMismatch));
    invalid = embedded;
    invalid.back() ^= 1;
    check_fallback(load(invalid), embedded, atlas_error_name(AtlasError::BadChecksum));
    invalid = embedded;
    invalid[4] = 2;
    check_fallback(load(invalid), embedded, atlas_error_name(AtlasError::UnsupportedVersion));
    return TEST_MAIN_RESULT();
}
