// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Research code for #145 (scratch build only; see apply_variant.sh). Renders a deterministic camera
// sweep of the lines-only globe and reports a checksum per frame, optionally writing all frames
// or comparing them with an earlier dump pixel by pixel. With variant 5 it also logs every
// argument and radius that inverse_length() receives.
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "globe/embedded_atlas.hpp"
#include "globe/render.hpp"
#include "render_variant.hpp"

using namespace globe;

int main(int argc, char** argv) {
    const char* write_path = nullptr;
    const char* compare_path = nullptr;
    const char* log_path = nullptr;
    int timing_reps = 0;
    const char* only_radii = nullptr; // comma list, e.g. "220,440"
    for (int i = 1; i + 1 < argc; i += 2) {
        if (!std::strcmp(argv[i], "--write"))
            write_path = argv[i + 1];
        else if (!std::strcmp(argv[i], "--compare"))
            compare_path = argv[i + 1];
        else if (!std::strcmp(argv[i], "--log"))
            log_path = argv[i + 1];
        else if (!std::strcmp(argv[i], "--time"))
            timing_reps = std::atoi(argv[i + 1]);
        else if (!std::strcmp(argv[i], "--radii"))
            only_radii = argv[i + 1];
        else
            return 2;
    }
    auto atlas = Atlas::parse(kEmbeddedAtlasData, kEmbeddedAtlasSize);
    if (!atlas)
        return 1;
    auto scene = Scene::create(*atlas);
    auto fb = Framebuffer::create(640, 480);
    if (!scene || !fb)
        return 1;
    std::FILE* wf = write_path ? std::fopen(write_path, "wb") : nullptr;
    std::FILE* cf = compare_path ? std::fopen(compare_path, "rb") : nullptr;
    if ((write_path && !wf) || (compare_path && !cf))
        return 1;
    if (log_path)
        rsqrt_lab::g_log = std::fopen(log_path, "wb");

    const double lats[] = {-80, -45, -10, 0, 30, 60, 85};
    const double radii[] = {55, 110, 220, 440, 880, 1760, 2560};
    const double headings[] = {0, 37};
    std::uint64_t chain = 14695981039346656037ULL;
    std::uint64_t cameras = 0, lines = 0, frames_diff = 0, pixels_diff = 0, worst_frame = 0;
    std::vector<std::uint16_t> frame(640 * 480), ref(640 * 480);
    double render_s = 0;
    struct Per {
        unsigned long frames = 0, visible = 0, calls = 0, steps_sqrt = 0, lines = 0;
    } per[8];
    int radius_index = -1;
    for (double radius : radii) {
        ++radius_index;
        if (only_radii) {
            char key[32];
            std::snprintf(key, sizeof key, ",%g,", radius);
            std::string list = std::string(",") + only_radii + ",";
            if (list.find(key) == std::string::npos)
                continue;
        }
        for (double lat : lats) {
            for (int li = 0; li < 18; ++li) {
                const double lon = -170.0 + 20.0 * li;
                for (double heading : headings) {
                    const auto camera = make_camera({{lat, lon}, heading, radius, 640, 480});
                    if (!camera)
                        return 1;
                    rsqrt_lab::g_radius = radius;
                    const int reps = timing_reps > 0 ? timing_reps : 1;
                    RenderStats st{};
                    const auto t0 = std::chrono::steady_clock::now();
                    for (int r = 0; r < reps; ++r)
                        st = render_globe(*fb, *camera, *scene, kDefaultPalette, Layers{}, true);
                    const auto t1 = std::chrono::steady_clock::now();
                    render_s += std::chrono::duration<double>(t1 - t0).count() / reps;
                    lines += st.lines_drawn;
                    {
                        Per& p = per[radius_index];
                        ++p.frames;
                        p.visible += st.segments_visible;
                        p.lines += st.lines_drawn;
                        p.calls = rsqrt_lab::g_calls;
                        p.steps_sqrt = rsqrt_lab::g_steps_sqrt;
                    }
                    std::uint64_t h = 14695981039346656037ULL;
                    for (int y = 0; y < 480; ++y)
                        for (int x = 0; x < 640; ++x) {
                            const std::uint16_t p = static_cast<std::uint16_t>(fb->pixel(x, y));
                            frame[static_cast<std::size_t>(y) * 640 + x] = p;
                            h = (h ^ p) * 1099511628211ULL;
                        }
                    chain = (chain ^ h) * 1099511628211ULL;
                    if (wf) { // little-endian on disk so dumps from any target compare
                        for (std::uint16_t p : frame) {
                            const unsigned char b[2] = {static_cast<unsigned char>(p & 255),
                                                        static_cast<unsigned char>(p >> 8)};
                            std::fwrite(b, 1, 2, wf);
                        }
                    }
                    if (cf) {
                        {
                            std::vector<unsigned char> raw(ref.size() * 2);
                            if (std::fread(raw.data(), 1, raw.size(), cf) != raw.size())
                                return 3;
                            for (std::size_t i = 0; i < ref.size(); ++i)
                                ref[i] =
                                    static_cast<std::uint16_t>(raw[2 * i] | (raw[2 * i + 1] << 8));
                        }
                        std::uint64_t d = 0;
                        for (std::size_t i = 0; i < frame.size(); ++i)
                            d += frame[i] != ref[i];
                        if (d) {
                            ++frames_diff;
                            pixels_diff += d;
                            if (d > worst_frame)
                                worst_frame = d;
                        }
                    }
                    ++cameras;
                }
            }
        }
    }
    if (wf)
        std::fclose(wf);
    if (cf)
        std::fclose(cf);
    if (rsqrt_lab::g_log)
        std::fclose(rsqrt_lab::g_log);
    std::printf("{\"cameras\":%llu,\"lines_drawn\":%llu,\"inverse_length_calls\":%lu,"
                "\"chain_hash\":\"%016llx\",\"frames_differing\":%llu,\"pixels_differing\":%llu,"
                "\"worst_frame_pixels\":%llu,\"render_seconds_total\":%.6f}\n",
                (unsigned long long)cameras, (unsigned long long)lines, rsqrt_lab::g_calls,
                (unsigned long long)chain, (unsigned long long)frames_diff,
                (unsigned long long)pixels_diff, (unsigned long long)worst_frame, render_s);
    // cumulative counters were sampled after each frame; convert to per-radius totals
    std::printf("{\"by_radius\":[");
    unsigned long pc = 0, ps = 0;
    for (int i = 0; i < 7; ++i) {
        if (!per[i].frames)
            continue;
        std::printf("{\"radius\":%g,\"frames\":%lu,\"segments_visible\":%lu,\"lines_drawn\":%lu,"
                    "\"inverse_length_calls\":%lu,\"sqrt_step_count_arcs\":%lu},",
                    radii[i], per[i].frames, per[i].visible, per[i].lines, per[i].calls - pc,
                    per[i].steps_sqrt - ps);
        pc = per[i].calls;
        ps = per[i].steps_sqrt;
    }
    std::printf("{}]}\n");
    return 0;
}
