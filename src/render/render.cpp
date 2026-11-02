// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/render.hpp"

#include <algorithm>
#include <cmath>
#include <new>

#include "globe/raster.hpp"

namespace globe {

namespace {

// A straight piece is drawn once the arc is within this many pixels of the true curve's chord.
constexpr double kMaxChordPx = 3.0;
constexpr std::size_t kMaxSubdivisions = 256;
constexpr int kGraticuleSpacingDeg = 30;
constexpr int kGraticuleSampleDeg = 3;

void draw_screen_line(Framebuffer& fb, const Camera& cam, Vec3 a, Vec3 b, Rgb565 color,
                      RenderStats& stats) {
    const ScreenPoint pa = to_screen(cam, a);
    const ScreenPoint pb = to_screen(cam, b);
    draw_line(fb, pa.x, pa.y, pb.x, pb.y, color);
    ++stats.lines_drawn;
}

// 1 / sqrt(l2) for l2 in [0.5, 1]: two Newton steps from 1 (relative error under 2e-3 after one,
// under 1e-5 after two), far cheaper than a square root on CPUs without a hardware one. Callers
// pass the squared length of a point on a chord, which only dips below 0.5 for arcs over 90
// degrees; those take the exact path.
double inverse_length(double l2) {
    if (l2 < 0.5) {
        return 1.0 / std::sqrt(l2);
    }
    double s = 1.5 - 0.5 * l2;
    s *= 1.5 - 0.5 * l2 * s * s;
    return s;
}

// Draws the shorter great-circle arc between two camera-space unit vectors.
//
// Long arcs are cut into pieces no longer than kMaxChordPx on screen. The piece count comes from
// the squared chord length (one square root, only for long arcs), and the interior points are the
// normalized points of the chord: they lie exactly on the great circle, though not evenly spaced
// in angle. That avoids the sine, cosine, arctangent and square roots an even angular spacing
// would need for every point.
void draw_arc(Framebuffer& fb, const Camera& cam, Vec3 a, Vec3 b, Rgb565 color,
              RenderStats& stats) {
    ++stats.segments;
    const std::optional<Arc> arc = clip_arc_to_front(a, b);
    if (!arc) {
        return;
    }
    ++stats.segments_visible;
    const double max_step = kMaxChordPx / cam.radius_px;
    const double max_step2 = max_step * max_step;
    const Vec3 d = arc->b - arc->a;
    const double len2 = dot(d, d);
    if (len2 <= max_step2) {
        draw_screen_line(fb, cam, arc->a, arc->b, color, stats);
        return;
    }
    const double ratio2 = len2 / max_step2;
    std::size_t steps = 2;
    if (ratio2 <= 64.0) {
        while (static_cast<double>(steps * steps) < ratio2) {
            ++steps;
        }
    } else {
        const double wanted = std::ceil(std::sqrt(ratio2));
        steps = wanted < static_cast<double>(kMaxSubdivisions)
                    ? static_cast<std::size_t>(wanted)
                    : kMaxSubdivisions; // very long arcs at high zoom get coarser, not slower
    }
    const double inv_steps = 1.0 / static_cast<double>(steps);
    ScreenPoint prev = to_screen(cam, arc->a);
    for (std::size_t i = 1; i < steps; ++i) {
        const double t = static_cast<double>(i) * inv_steps;
        Vec3 v = arc->a * (1.0 - t) + arc->b * t;
        v = v * inverse_length(dot(v, v));
        const ScreenPoint p = to_screen(cam, v);
        draw_line(fb, prev.x, prev.y, p.x, p.y, color);
        ++stats.lines_drawn;
        prev = p;
    }
    const ScreenPoint last = to_screen(cam, arc->b);
    draw_line(fb, prev.x, prev.y, last.x, last.y, color);
    ++stats.lines_drawn;
}

constexpr int kParallels = 5;         // latitudes -60, -30, 0, 30, 60
constexpr int kParallelSamples = 121; // longitudes -180 to 180 every kGraticuleSampleDeg
constexpr int kMeridians = 12;        // longitudes -180 to 150 every kGraticuleSpacingDeg
constexpr int kMeridianSamples = 61;  // latitudes -90 to 90 every kGraticuleSampleDeg
constexpr std::size_t kGraticulePoints = static_cast<std::size_t>(kParallels) * kParallelSamples +
                                         static_cast<std::size_t>(kMeridians) * kMeridianSamples;

Vec3 parallel_point(int parallel, int sample) {
    const double lat = -60.0 + kGraticuleSpacingDeg * parallel;
    const double lon = -180.0 + kGraticuleSampleDeg * sample;
    return to_unit_vector({lat, lon});
}

Vec3 meridian_point(int meridian, int sample) {
    const double lon = -180.0 + kGraticuleSpacingDeg * meridian;
    const double lat = -90.0 + kGraticuleSampleDeg * sample;
    return to_unit_vector({lat, lon});
}

// Draws the graticule from any source of world-space sample points. Parallels are not great
// circles, so they are sampled finely enough that each piece is a short arc; meridians are great
// circles drawn one sample at a time, pole to pole.
template <typename ParallelPoint, typename MeridianPoint>
void draw_graticule(Framebuffer& fb, const Camera& cam, Rgb565 color, RenderStats& stats,
                    ParallelPoint parallel, MeridianPoint meridian) {
    for (int p = 0; p < kParallels; ++p) {
        Vec3 prev = to_camera_space(cam, parallel(p, 0));
        for (int s = 1; s < kParallelSamples; ++s) {
            const Vec3 cur = to_camera_space(cam, parallel(p, s));
            draw_arc(fb, cam, prev, cur, color, stats);
            prev = cur;
        }
    }
    for (int m = 0; m < kMeridians; ++m) {
        Vec3 prev = to_camera_space(cam, meridian(m, 0));
        for (int s = 1; s < kMeridianSamples; ++s) {
            const Vec3 cur = to_camera_space(cam, meridian(m, s));
            draw_arc(fb, cam, prev, cur, color, stats);
            prev = cur;
        }
    }
}

// Draws one layer of the atlas; `vertex(line, first_point, i)` supplies each world-space vertex.
// Each vertex is transformed once and carried to the next segment.
template <typename VertexAt>
void draw_atlas_layer(Framebuffer& fb, const Camera& cam, const Atlas& atlas, AtlasLayer layer,
                      Rgb565 color, RenderStats& stats, VertexAt vertex) {
    AtlasCursor cursor;
    AtlasPolyline line{AtlasLayer::Coastline, 0, nullptr};
    for (;;) {
        const std::uint32_t first = cursor.first_point;
        if (!atlas.next(cursor, line)) {
            break;
        }
        if (line.layer != layer) {
            continue;
        }
        Vec3 prev = to_camera_space(cam, vertex(line, first, 0));
        for (std::uint16_t i = 1; i < line.point_count; ++i) {
            const Vec3 cur = to_camera_space(cam, vertex(line, first, i));
            draw_arc(fb, cam, prev, cur, color, stats);
            prev = cur;
        }
    }
}

} // namespace

std::optional<Scene> Scene::create(const Atlas& atlas) {
    const std::size_t total = static_cast<std::size_t>(atlas.point_count()) + kGraticulePoints;
    std::unique_ptr<Vec3[]> storage(new (std::nothrow) Vec3[total]);
    if (!storage) {
        return std::nullopt;
    }
    Vec3* out = storage.get();
    AtlasCursor cursor;
    AtlasPolyline line{AtlasLayer::Coastline, 0, nullptr};
    while (atlas.next(cursor, line)) {
        for (std::uint16_t i = 0; i < line.point_count; ++i) {
            *out++ = to_unit_vector(line.point(i));
        }
    }
    for (int p = 0; p < kParallels; ++p) {
        for (int s = 0; s < kParallelSamples; ++s) {
            *out++ = parallel_point(p, s);
        }
    }
    for (int m = 0; m < kMeridians; ++m) {
        for (int s = 0; s < kMeridianSamples; ++s) {
            *out++ = meridian_point(m, s);
        }
    }
    return Scene(atlas, std::move(storage));
}

std::size_t Scene::bytes() const {
    return (static_cast<std::size_t>(atlas_.point_count()) + kGraticulePoints) * sizeof(Vec3);
}

RenderStats render_globe(Framebuffer& fb, const Camera& camera, const Scene& scene,
                         const Palette& palette, const Layers& layers) {
    RenderStats stats;
    fb.clear(palette.background);
    fill_disc(fb, camera.center_x, camera.center_y, camera.radius_px, palette.ocean);
    if (layers.graticule) {
        const Vec3* grid = scene.graticule();
        const Vec3* meridians = grid + static_cast<std::size_t>(kParallels) * kParallelSamples;
        draw_graticule(
            fb, camera, palette.graticule, stats,
            [grid](int p, int s) {
                return grid[static_cast<std::size_t>(p * kParallelSamples + s)];
            },
            [meridians](int m, int s) {
                return meridians[static_cast<std::size_t>(m * kMeridianSamples + s)];
            });
    }
    const Vec3* vertices = scene.vertices();
    const auto vertex = [vertices](const AtlasPolyline&, std::uint32_t first, std::uint16_t i) {
        return vertices[first + i];
    };
    if (layers.coastlines) {
        draw_atlas_layer(fb, camera, scene.atlas(), AtlasLayer::Coastline, palette.coastline, stats,
                         vertex);
    }
    if (layers.borders) {
        draw_atlas_layer(fb, camera, scene.atlas(), AtlasLayer::Border, palette.border, stats,
                         vertex);
    }
    return stats;
}

RenderStats render_globe(Framebuffer& fb, const Camera& camera, const Atlas& atlas,
                         const Palette& palette, const Layers& layers) {
    RenderStats stats;
    fb.clear(palette.background);
    fill_disc(fb, camera.center_x, camera.center_y, camera.radius_px, palette.ocean);
    if (layers.graticule) {
        draw_graticule(fb, camera, palette.graticule, stats, parallel_point, meridian_point);
    }
    const auto vertex = [](const AtlasPolyline& line, std::uint32_t, std::uint16_t i) {
        return to_unit_vector(line.point(i));
    };
    if (layers.coastlines) {
        draw_atlas_layer(fb, camera, atlas, AtlasLayer::Coastline, palette.coastline, stats,
                         vertex);
    }
    if (layers.borders) {
        draw_atlas_layer(fb, camera, atlas, AtlasLayer::Border, palette.border, stats, vertex);
    }
    return stats;
}

} // namespace globe
