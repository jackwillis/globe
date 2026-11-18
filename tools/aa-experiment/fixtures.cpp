// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Research-only fixture extraction. Production files and entry points are unchanged.
#include "aa.hpp"
#include "globe/atlas.hpp"
#include "globe/elevation.hpp"
#include "globe/landclasses.hpp"
#include "globe/raster.hpp"
#include "globe/render.hpp"
#include "globe/terrain.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#include <string>

using namespace globe;
namespace {
constexpr unsigned MaxPoints = 262144, MaxChains = 32768, MaxFeatures = 8192;
struct PackedChain {
    unsigned start = 0, count = 0;
    bool closed = false;
};
struct Feature {
    unsigned start = 0, count = 0, id = 0;
    Rgb565 color = 0;
};
struct Fixture {
    std::unique_ptr<ScreenPoint[]> points{new (std::nothrow) ScreenPoint[MaxPoints]};
    std::unique_ptr<PackedChain[]> chains{new (std::nothrow) PackedChain[MaxChains]};
    std::unique_ptr<aa_experiment::Chain[]> borrowed{new (std::nothrow)
                                                         aa_experiment::Chain[MaxChains]};
    std::unique_ptr<Feature[]> features{new (std::nothrow) Feature[MaxFeatures]};
    unsigned np = 0, nc = 0, nf = 0, current = MaxChains;
    bool ok() const { return points && chains && borrowed && features; }
    bool feature(Rgb565 color, unsigned id) {
        if (nf == MaxFeatures)
            return false;
        features[nf++] = {nc, 0, id, color};
        current = MaxChains;
        return true;
    }
    bool append(ScreenPoint a, ScreenPoint b) {
        if (current == MaxChains || points[np - 1].x != a.x || points[np - 1].y != a.y) {
            if (nc == MaxChains || np == MaxPoints)
                return false;
            current = nc;
            chains[nc++] = {np, 0, false};
            ++features[nf - 1].count;
            points[np++] = a;
            ++chains[current].count;
        }
        if (np == MaxPoints)
            return false;
        points[np++] = b;
        ++chains[current].count;
        return true;
    }
    void end_feature(bool source_closed) {
        auto& f = features[nf - 1];
        if (source_closed && f.count == 1) {
            auto& c = chains[f.start];
            const auto a = points[c.start], b = points[c.start + c.count - 1];
            c.closed = std::hypot(a.x - b.x, a.y - b.y) < 1e-8;
        }
        for (unsigned i = f.start; i < f.start + f.count; ++i)
            borrowed[i] = {{points.get() + chains[i].start, chains[i].count}, chains[i].closed};
    }
};
struct Bytes {
    std::unique_ptr<std::uint8_t[]> p;
    std::size_t n = 0;
    bool load(const std::string& path) {
        FILE* f = std::fopen(path.c_str(), "rb");
        if (!f)
            return false;
        if (std::fseek(f, 0, SEEK_END) != 0) {
            std::fclose(f);
            return false;
        }
        long s = std::ftell(f);
        if (s < 0 || s > 16 * 1024 * 1024) {
            std::fclose(f);
            return false;
        }
        std::rewind(f);
        n = static_cast<std::size_t>(s);
        p.reset(new (std::nothrow) std::uint8_t[n]);
        bool ok = p && std::fread(p.get(), 1, n, f) == n;
        ok = std::fclose(f) == 0 && ok;
        return ok;
    }
};
// Transcription of pinned render.cpp's arc path, preserving arithmetic order.
double inverse_length(double l2) {
    if (l2 < .5)
        return 1 / std::sqrt(l2);
    double s = 1.5 - .5 * l2;
    s *= 1.5 - .5 * l2 * s * s;
    return s;
}
bool arc(Fixture& f, const Camera& cam, Vec3 a, Vec3 b) {
    auto clipped = clip_arc_to_front(a, b);
    if (!clipped) {
        f.current = MaxChains;
        return true;
    }
    const double max_step = 3.0 / cam.radius_px, max_step2 = max_step * max_step;
    const Vec3 d = clipped->b - clipped->a;
    const double len2 = dot(d, d);
    if (a.z < 0)
        f.current = MaxChains;
    if (len2 <= max_step2) {
        if (!f.append(to_screen(cam, clipped->a), to_screen(cam, clipped->b)))
            return false;
    } else {
        const double ratio2 = len2 / max_step2;
        std::size_t steps = 2;
        if (ratio2 <= 64) {
            while (static_cast<double>(steps * steps) < ratio2)
                ++steps;
        } else {
            double wanted = std::ceil(std::sqrt(ratio2));
            steps = wanted < 256 ? static_cast<std::size_t>(wanted) : 256;
        }
        const double inv_steps = 1.0 / static_cast<double>(steps);
        ScreenPoint prev = to_screen(cam, clipped->a);
        for (std::size_t i = 1; i < steps; ++i) {
            double t = static_cast<double>(i) * inv_steps;
            Vec3 v = clipped->a * (1 - t) + clipped->b * t;
            v = v * inverse_length(dot(v, v));
            ScreenPoint p = to_screen(cam, v);
            if (!f.append(prev, p))
                return false;
            prev = p;
        }
        if (!f.append(prev, to_screen(cam, clipped->b)))
            return false;
    }
    if (b.z < 0)
        f.current = MaxChains;
    return true;
}
bool extract(Fixture& out, const Scene& scene, const Camera& cam, Palette palette) {
    if (!out.ok())
        return false;
    unsigned id = 0;
    for (unsigned family = 0; family < 2; ++family) {
        const unsigned count = family ? 12 : 5, samples = family ? 61 : 121,
                       offset = family ? 5 * 121 : 0;
        for (unsigned line = 0; line < count; ++line) {
            if (!out.feature(palette.graticule, id++))
                return false;
            auto at = [&](unsigned i) {
                return to_camera_space(cam, scene.graticule()[offset + line * samples + i]);
            };
            Vec3 prev = at(0);
            for (unsigned i = 1; i < samples; ++i) {
                Vec3 next = at(i);
                if (!arc(out, cam, prev, next))
                    return false;
                prev = next;
            }
            out.end_feature(family == 0);
        }
    }
    for (AtlasLayer layer : {AtlasLayer::Coastline, AtlasLayer::Border}) {
        AtlasCursor cursor;
        AtlasPolyline line{};
        for (;;) {
            unsigned index = cursor.index, first = cursor.first_point;
            if (!scene.atlas().next(cursor, line))
                break;
            if (line.layer != layer)
                continue;
            const auto& cap = scene.caps()[index];
            if (dot(cap.axis, cam.forward) < -cap.sin_radius - 1e-12)
                continue;
            if (!out.feature(layer == AtlasLayer::Coastline ? palette.coastline : palette.border,
                             id++))
                return false;
            auto at = [&](unsigned i) { return to_camera_space(cam, scene.vertices()[first + i]); };
            Vec3 prev = at(0);
            for (unsigned i = 1; i < line.point_count; ++i) {
                Vec3 next = at(i);
                if (!arc(out, cam, prev, next))
                    return false;
                prev = next;
            }
            const auto a = line.point(0), b = line.point(line.point_count - 1);
            out.end_feature(a.lat_deg == b.lat_deg && a.lon_deg == b.lon_deg);
        }
    }
    return true;
}
bool equal(const Framebuffer& a, const Framebuffer& b) {
    return std::equal(a.data(), a.data() + a.width() * a.height(), b.data());
}
void replay_a(Framebuffer& fb, const Fixture& f) {
    for (unsigned i = 0; i < f.nf; ++i) {
        auto feature = f.features[i];
        for (unsigned j = feature.start; j < feature.start + feature.count; ++j) {
            auto c = f.chains[j];
            for (unsigned k = 1; k < c.count; ++k) {
                auto a = f.points[c.start + k - 1], b = f.points[c.start + k];
                draw_line(fb, a.x, a.y, b.x, b.y, feature.color);
            }
        }
    }
}
bool replay_b(Framebuffer& fb, const Fixture& f, aa_experiment::Workspace& ws) {
    for (unsigned i = 0; i < f.nf; ++i) {
        auto feature = f.features[i];
        if (aa_experiment::render_feature(fb, {f.borrowed.get() + feature.start, feature.count},
                                          feature.color, ws) != aa_experiment::Result::Ok)
            return false;
    }
    return true;
}
bool settled(Framebuffer& fb, const Camera& cam, const Scene& scene, TerrainScene& terrain,
             RenderQuality quality, unsigned budget) {
    RefinementJob job(cam, scene, kDefaultPalette, {}, quality, 1, &terrain);
    unsigned steps = 0;
    while (job.active() && ++steps < 10000000)
        job.step(fb, 1, budget);
    return !job.active();
}
void u16(FILE* f, std::uint16_t v) {
    std::fputc(v & 255, f);
    std::fputc(v >> 8, f);
}
void u32(FILE* f, std::uint32_t v) {
    for (unsigned i = 0; i < 4; ++i)
        std::fputc((v >> (8 * i)) & 255, f);
}
void f64(FILE* f, double x) {
    auto v = std::bit_cast<std::uint64_t>(x);
    for (unsigned i = 0; i < 8; ++i)
        std::fputc((v >> (8 * i)) & 255, f);
}
bool pixels(const std::string& path, const Framebuffer& fb) {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f)
        return false;
    for (int i = 0; i < fb.width() * fb.height(); ++i)
        u16(f, fb.data()[i]);
    bool ok = std::ferror(f) == 0;
    return std::fclose(f) == 0 && ok;
}
bool geometry(const std::string& path, const Fixture& data) {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f)
        return false;
    std::fwrite("PEAAAB1\0", 1, 8, f);
    u32(f, data.nf);
    for (unsigned i = 0; i < data.nf; ++i) {
        const auto feature = data.features[i];
        u32(f, feature.id);
        u16(f, feature.color);
        u32(f, feature.count);
        for (unsigned j = feature.start; j < feature.start + feature.count; ++j) {
            const auto c = data.chains[j];
            std::fputc(c.closed ? 1 : 0, f);
            u32(f, c.count);
            for (unsigned k = 0; k < c.count; ++k) {
                auto p = data.points[c.start + k];
                f64(f, p.x);
                f64(f, p.y);
            }
        }
    }
    bool ok = std::ferror(f) == 0;
    return std::fclose(f) == 0 && ok;
}
} // namespace
int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: aa_fixtures REPO_ROOT EXISTING_OUTPUT_DIRECTORY\n");
        return 2;
    }
    const std::string repo = argv[1], out = argv[2];
    Bytes atlas_bytes, elevation_bytes, class_bytes;
    if (!atlas_bytes.load(repo + "/data/atlas.gatl") ||
        !elevation_bytes.load(repo + "/data/elevation-1deg.gelv") ||
        !class_bytes.load(repo + "/data/landclasses-1deg.ggrd"))
        return 3;
    auto atlas = Atlas::parse(atlas_bytes.p.get(), atlas_bytes.n);
    auto elevation = Elevation::parse(elevation_bytes.p.get(), elevation_bytes.n);
    auto classes = LandClasses::decode(class_bytes.p.get(), class_bytes.n);
    if (!atlas || !elevation || !classes)
        return 3;
    auto scene = Scene::create(*atlas);
    auto terrain = TerrainScene::create(*elevation, &*classes);
    auto background = Framebuffer::create(640, 480), expected = Framebuffer::create(640, 480),
         a = Framebuffer::create(640, 480), b = Framebuffer::create(640, 480);
    auto ws = aa_experiment::Workspace::create(640, 480, 1024 * 1024 - 1);
    if (!scene || !terrain || !background || !expected || !a || !b || !ws)
        return 4;
    const LatLon centers[] = {{30, 10}, {62, 8}, {-3, 120}, {48, 15},
                              {0, 179}, {89, 0}, {-89, 0},  {-20, -150}};
    unsigned number = 0;
    for (unsigned view = 0; view < 8; ++view)
        for (double radius : {220., 440.})
            for (TerrainStyle style : {TerrainStyle::Map, TerrainStyle::Relief}) {
                auto cam = make_camera({centers[view], 0, radius, 640, 480});
                if (!cam)
                    return 5;
                RenderQuality q{3.0, true, 0, style};
                q.focus = {320, 240};
                q.terrain_only = true;
                if (!settled(*background, *cam, *scene, *terrain, q, 256))
                    return 5;
                for (unsigned budget : {1U, 7U}) {
                    if (!settled(*expected, *cam, *scene, *terrain, q, budget) ||
                        !equal(*background, *expected))
                        return 6;
                }
                q.terrain_only = false;
                if (!settled(*expected, *cam, *scene, *terrain, q, 256))
                    return 5;
                Palette palette = palette_for_style(kDefaultPalette, style);
                if (style != TerrainStyle::Map)
                    palette.border = rgb565(0, 0, 0);
                Fixture fixture;
                if (!extract(fixture, *scene, *cam, palette))
                    return 7;
                std::memcpy(a->data(), background->data(), a->byte_size());
                replay_a(*a, fixture);
                if (!equal(*a, *expected)) {
                    unsigned mismatch = 0;
                    for (int i = 0; i < 640 * 480; ++i)
                        if (a->data()[i] != expected->data()[i])
                            ++mismatch;
                    std::fprintf(stderr, "view=%u radius=%.0f style=%u replay mismatches=%u\n",
                                 view, radius, static_cast<unsigned>(style), mismatch);
                    return 8;
                }
                std::memcpy(b->data(), background->data(), b->byte_size());
                if (!replay_b(*b, fixture, *ws))
                    return 9;
                char name[80];
                std::snprintf(name, sizeof name, "view%u-r%.0f-%s", view, radius,
                              style == TerrainStyle::Map ? "map" : "relief");
                const std::string stem = out + "/" + name;
                if (!pixels(stem + "-background.rgb565", *background) ||
                    !pixels(stem + "-A.rgb565", *a) || !pixels(stem + "-B.rgb565", *b) ||
                    !geometry(stem + ".geometry", fixture))
                    return 10;
                std::printf("{\"fixture\":\"%s\",\"lat\":%.0f,\"lon\":%.0f,\"radius\":%.0f,"
                            "\"features\":%u,\"chains\":%u,\"points\":%u,\"background_budgets\":[1,"
                            "7,256],\"A_replay_equal\":true,\"B_scratch\":%zu}\n",
                            name, centers[view].lat_deg, centers[view].lon_deg, radius, fixture.nf,
                            fixture.nc, fixture.np, ws->bytes());
                std::fflush(stdout);
                ++number;
            }
    std::fprintf(stderr,
                 "Verified %u fixtures; no geographic quality scores or timings computed.\n",
                 number);
    return number == 32 ? 0 : 11;
}
