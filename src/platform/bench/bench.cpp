// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Headless benchmark: replays a recorded message trace through update() and render_globe(),
// timing each frame, and prints one JSON object. No display, window system or network.
//
//   globe-bench [--atlas FILE] [--elevation FILE] [--size WxH] [--repeat N] [--convert] [--layers
//   gcb] [--reference] TRACE
//
// The embedded atlas is used unless --atlas is given. With --convert each frame is also packed
// into a 32-bit X-style pixel layout, to include that cost. With --layers, only the listed layers
// (g graticule, c coastlines, b borders; "-" for none) are drawn, whatever the trace toggles, to
// attribute frame cost to layers. Times are wall-clock milliseconds
// from CLOCK_MONOTONIC; see the "limits" field of the output for what is and is not measured.
#include <sys/resource.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "cli.hpp"
#include "clock.hpp"
#include "elevation_packs.hpp"
#include "frame_stats.hpp"
#include "globe/app.hpp"
#include "globe/atlas.hpp"
#include "globe/embedded_atlas.hpp"
#include "globe/embedded_elevation.hpp"
#include "globe/terrain.hpp"
#include "globe/trace.hpp"
#include "pixel_format.hpp"

using namespace globe;

namespace {

bool read_file(const char* path, std::vector<std::uint8_t>& out) {
    std::FILE* f = std::fopen(path, "rb");
    if (f == nullptr) {
        return false;
    }
    std::uint8_t buf[8192];
    std::size_t n = 0;
    out.clear();
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) {
        out.insert(out.end(), buf, buf + n);
    }
    std::fclose(f);
    return true;
}

double ms(std::uint64_t ns) {
    return static_cast<double>(ns) / 1.0e6;
}

void print_summary(const char* name, const Summary& s, bool last) {
    std::printf("  \"%s\": {\"count\": %zu, \"min\": %.4f, \"mean\": %.4f, \"p50\": %.4f, "
                "\"p95\": %.4f, \"max\": %.4f}%s\n",
                name, s.count, s.min, s.mean, s.p50, s.p95, s.max, last ? "" : ",");
}

PixelFormat host_x_format() {
    // What a 24-bit-depth X server typically asks for: 32 bits per pixel, byte order = host's.
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return {32, 0x00FF0000U, 0x0000FF00U, 0x000000FFU, true};
#else
    return {32, 0x00FF0000U, 0x0000FF00U, 0x000000FFU, false};
#endif
}

} // namespace

int main(int argc, char** argv) {
    const std::uint64_t t_start = monotonic_ns();
    const char* atlas_path = nullptr;
    std::array<const char*, 3> elevation_paths{};
    unsigned elevation_count = 0;
    const char* dump_path = nullptr;
    const char* trace_path = nullptr;
    int width = 640;
    int height = 480;
    int repeat = 3;
    bool convert = false;
    bool refine = false;
    bool reference_path = false;
    bool force_layers = false;
    Layers forced{};
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--atlas") == 0 && i + 1 < argc) {
            atlas_path = argv[++i];
        } else if (std::strcmp(argv[i], "--elevation") == 0 && i + 1 < argc) {
            if (elevation_count == elevation_paths.size())
                return 2;
            elevation_paths[elevation_count++] = argv[++i];
        } else if (std::strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
            if (!parse_size_arg(argv[++i], kMaxViewportDimension, width, height)) {
                std::fprintf(stderr, "bad --size\n");
                return 2;
            }
        } else if (std::strcmp(argv[i], "--repeat") == 0 && i + 1 < argc) {
            if (!parse_int_arg(argv[++i], 1, 1000, repeat)) {
                std::fprintf(stderr, "bad --repeat (1..1000)\n");
                return 2;
            }
        } else if (std::strcmp(argv[i], "--reference") == 0) {
            reference_path = true; // compute vertex vectors per frame instead of using the Scene
        } else if (std::strcmp(argv[i], "--dump-ppm") == 0 && i + 1 < argc) {
            dump_path = argv[++i];
        } else if (std::strcmp(argv[i], "--refine") == 0) {
            refine = true;
        } else if (std::strcmp(argv[i], "--convert") == 0) {
            convert = true;
        } else if (std::strcmp(argv[i], "--layers") == 0 && i + 1 < argc) {
            const char* letters = argv[++i];
            forced = Layers{false, false, false};
            force_layers = true;
            for (const char* c = letters; *c != '\0'; ++c) {
                if (*c == 'g') {
                    forced.graticule = true;
                } else if (*c == 'c') {
                    forced.coastlines = true;
                } else if (*c == 'b') {
                    forced.borders = true;
                } else if (*c != '-') {
                    std::fprintf(stderr, "bad --layers (use g, c, b or -)\n");
                    return 2;
                }
            }
        } else if (argv[i][0] != '-' && trace_path == nullptr) {
            trace_path = argv[i];
        } else {
            std::fprintf(
                stderr,
                "usage: globe-bench [--atlas FILE] [--elevation FILE] [--size WxH] [--repeat N] "
                "[--convert] [--layers gcb] TRACE\n");
            return 2;
        }
    }
    if (trace_path == nullptr || repeat < 1 || repeat > 1000 || width < 1 || height < 1 ||
        width > kMaxViewportDimension || height > kMaxViewportDimension) {
        std::fprintf(stderr, "bad arguments\n");
        return 2;
    }

    std::vector<std::uint8_t> trace_bytes;
    if (!read_file(trace_path, trace_bytes)) {
        std::fprintf(stderr, "cannot read %s\n", trace_path);
        return 2;
    }
    std::vector<Msg> messages;
    {
        const std::string text(trace_bytes.begin(), trace_bytes.end());
        std::size_t pos = 0;
        while (pos < text.size()) {
            std::size_t end = text.find('\n', pos);
            if (end == std::string::npos) {
                end = text.size();
            }
            if (const auto msg = parse_message(std::string_view(text).substr(pos, end - pos))) {
                messages.push_back(*msg);
            }
            pos = end + 1;
        }
    }

    std::vector<std::uint8_t> atlas_file;
    const std::uint8_t* atlas_data = kEmbeddedAtlasData;
    std::size_t atlas_size = kEmbeddedAtlasSize;
    if (atlas_path != nullptr) {
        if (!read_file(atlas_path, atlas_file)) {
            std::fprintf(stderr, "cannot read %s\n", atlas_path);
            return 2;
        }
        atlas_data = atlas_file.data();
        atlas_size = atlas_file.size();
    }
    const std::uint64_t t_parse0 = monotonic_ns();
    AtlasError err = AtlasError::Ok;
    const auto atlas = Atlas::parse(atlas_data, atlas_size, &err);
    const std::uint64_t t_parse1 = monotonic_ns();
    if (!atlas) {
        std::fprintf(stderr, "atlas: %s\n", atlas_error_name(err));
        return 2;
    }

    const std::uint64_t t_scene0 = monotonic_ns();
    const auto scene = Scene::create(*atlas);
    const std::uint64_t t_scene1 = monotonic_ns();
    if (!scene) {
        std::fprintf(stderr, "scene allocation failed\n");
        return 2;
    }
    ElevationPacks packs;
    if (!packs.initialize(kEmbeddedElevationData, kEmbeddedElevationSize))
        return 2;
    for (unsigned i = 0; i < elevation_count; ++i)
        if (!packs.import_file(elevation_paths[i])) {
            std::fprintf(stderr, "globe-bench: %s\n", packs.error());
            return 2;
        }
    auto fb = Framebuffer::create(width, height);
    if (!fb) {
        std::fprintf(stderr, "framebuffer allocation failed\n");
        return 2;
    }
    const PixelFormat format = host_x_format();
    std::vector<std::uint8_t> packed;
    const std::size_t stride = pixel_row_bytes(format, width);
    if (convert) {
        packed.resize(stride * static_cast<std::size_t>(height));
    }

    // Timer overhead: the smallest gap between two back-to-back reads.
    std::uint64_t overhead = ~0ULL;
    for (int i = 0; i < 1000; ++i) {
        const std::uint64_t a = monotonic_ns();
        const std::uint64_t b = monotonic_ns();
        overhead = b - a < overhead ? b - a : overhead;
    }

    std::vector<double> render_ms;
    std::vector<double> convert_ms;
    std::vector<double> frame_segments;
    std::vector<double> frame_lines;
    unsigned selected_level = 0;
    double refine_ms = 0.0;
    double max_refine_step_ms = 0.0;
    unsigned refine_steps = 0;
    double first_frame_ms = 0.0;
    std::uint64_t checksum = 1469598103934665603ULL; // FNV-1a over final frames, as a sanity tag

    for (int rep = 0; rep < repeat; ++rep) {
        Model model = initial_model(width, height);
        packs.register_levels(model);
        if (force_layers) {
            model.layers = forced;
        }
        // The first frame of every repetition is the initial exposure; later frames follow
        // the trace's own redraw requests.
        bool need_frame = true;
        std::size_t next = 0;
        for (;;) {
            if (need_frame) {
                const std::uint64_t t0 = monotonic_ns();
                const RenderStats rs = reference_path
                                           ? render_globe(*fb, model.camera, *atlas,
                                                          kDefaultPalette, effective_layers(model))
                                           : render_globe(*fb, model.camera, *scene,
                                                          kDefaultPalette, effective_layers(model));
                const std::uint64_t t1 = monotonic_ns();
                render_ms.push_back(ms(t1 - t0));
                frame_segments.push_back(rs.segments_visible);
                frame_lines.push_back(rs.lines_drawn);
                if (convert) {
                    const std::uint64_t c0 = monotonic_ns();
                    convert_pixels(*fb, format, packed.data(), stride);
                    convert_ms.push_back(ms(monotonic_ns() - c0));
                }
                if (rep == 0 && render_ms.size() == 1) {
                    first_frame_ms = ms(t1 - t_start);
                }
                need_frame = false;
            }
            if (next >= messages.size()) {
                break;
            }
            const Effects fx = update(model, messages[next++]);
            if (force_layers) {
                model.layers = forced;
            }
            need_frame = fx.redraw;
            if (fx.quit) {
                break;
            }
        }
        selected_level = quality_for(model).elevation_level;
        if (refine) {
            model.stage = QualityStage::Refine;
            RefinementJob job(model.camera, *scene, kDefaultPalette, effective_layers(model),
                              quality_for(model), model.generation, packs.for_model(model));
            const auto started = monotonic_ns();
            while (job.active()) {
                const auto step_start = monotonic_ns();
                job.step(*fb, model.generation);
                max_refine_step_ms = std::max(max_refine_step_ms, ms(monotonic_ns() - step_start));
                ++refine_steps;
            }
            refine_ms += ms(monotonic_ns() - started);
        }
        for (int y = 0; y < fb->height(); y += 7) {
            for (int x = 0; x < fb->width(); x += 5) {
                checksum = (checksum ^ fb->pixel(x, y)) * 1099511628211ULL;
            }
        }
    }
    const std::uint64_t t_end = monotonic_ns();
    if (dump_path != nullptr) {
        auto* out = std::fopen(dump_path, "wb");
        if (!out)
            return 2;
        std::fprintf(out, "P6\n%d %d\n255\n", fb->width(), fb->height());
        for (int y = 0; y < fb->height(); ++y)
            for (int x = 0; x < fb->width(); ++x) {
                const auto pixel = fb->pixel(x, y);
                std::fputc(((pixel >> 11) * 255 + 15) / 31, out);
                std::fputc((((pixel >> 5) & 63) * 255 + 31) / 63, out);
                std::fputc(((pixel & 31) * 255 + 15) / 31, out);
            }
        if (std::fclose(out) != 0)
            return 2;
    }

    rusage usage{};
    getrusage(RUSAGE_SELF, &usage);

    const Summary sr = summarize(render_ms.data(), render_ms.size());
    const Summary sc = summarize(convert_ms.data(), convert_ms.size());
    const Summary ss = summarize(frame_segments.data(), frame_segments.size());
    const Summary sl = summarize(frame_lines.data(), frame_lines.size());

#ifdef __OPTIMIZE__
    const char* optimized = "yes";
#else
    const char* optimized = "no";
#endif
    std::printf("{\n");
    std::printf("  \"tool\": \"globe-bench\",\n");
    std::printf("  \"compiler\": \"%s\",\n", __VERSION__);
    std::printf("  \"optimized\": \"%s\",\n", optimized);
    std::printf("  \"viewport\": \"%dx%d\",\n", width, height);
    std::printf("  \"layers\": \"%s\",\n", force_layers ? "forced" : "as traced");
    std::printf("  \"trace_messages\": %zu,\n", messages.size());
    std::printf("  \"repeats\": %d,\n", repeat);
    std::printf("  \"atlas_bytes\": %zu,\n", atlas_size);
    std::printf("  \"atlas_polylines\": %u,\n", atlas->polyline_count());
    std::printf("  \"atlas_points\": %u,\n", atlas->point_count());
    std::printf("  \"atlas_parse_ms\": %.4f,\n", ms(t_parse1 - t_parse0));
    std::printf("  \"renderer\": \"%s\",\n", reference_path ? "reference" : "scene");
    std::printf("  \"scene_build_ms\": %.4f,\n  \"scene_bytes\": %zu,\n", ms(t_scene1 - t_scene0),
                scene->bytes());
    std::printf("  \"first_frame_ms_since_main\": %.4f,\n", first_frame_ms);
    std::printf("  \"total_ms\": %.2f,\n", ms(t_end - t_start));
    std::printf("  \"timer_overhead_ns\": %llu,\n", static_cast<unsigned long long>(overhead));
    std::printf("  \"peak_rss_kb\": %ld,\n", usage.ru_maxrss);
    std::printf("  \"frame_buffer_bytes\": %zu,\n", fb->byte_size());
    std::printf("  \"frames\": %zu,\n", render_ms.size());
    std::printf("  \"elevation_pack_count\": %u,\n  \"selected_elevation_level\": %u,\n",
                packs.count(), selected_level);
    std::printf("  \"terrain_cache_bytes\": %zu,\n", packs.bytes());
    std::printf("  \"refine_ms\": %.4f,\n  \"max_refine_step_ms\": %.4f,\n"
                "  \"refine_steps\": %u,\n",
                refine_ms / repeat, max_refine_step_ms, refine_steps);
    print_summary("render_ms", sr, false);
    if (convert) {
        print_summary("convert_ms", sc, false);
    }
    print_summary("visible_segments_per_frame", ss, false);
    print_summary("lines_per_frame", sl, false);
    std::printf("  \"frame_checksum\": \"%016llx\",\n", static_cast<unsigned long long>(checksum));
    std::printf("  \"limits\": \"render and optional pixel packing only; no X11 submission, no "
                "display, no idle CPU; peak_rss_kb is ru_maxrss as the OS reports it (kilobytes "
                "on Linux and NetBSD); times are wall clock, not CPU time\"\n");
    std::printf("}\n");
    return 0;
}
