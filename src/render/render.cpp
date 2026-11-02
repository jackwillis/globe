// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/render.hpp"

#include <algorithm>
#include <cmath>

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

// Draws the shorter great-circle arc between two camera-space unit vectors.
void draw_arc(Framebuffer& fb, const Camera& cam, Vec3 a, Vec3 b, Rgb565 color,
              RenderStats& stats) {
    ++stats.segments;
    const std::optional<Arc> arc = clip_arc_to_front(a, b);
    if (!arc) {
        return;
    }
    ++stats.segments_visible;
    const double max_step = kMaxChordPx / cam.radius_px;
    const Vec3 d = arc->b - arc->a;
    if (dot(d, d) <= max_step * max_step) {
        draw_screen_line(fb, cam, arc->a, arc->b, color, stats);
        return;
    }
    Vec3 pts[kMaxSubdivisions + 1];
    // Never ask for more steps than the buffer holds; very long arcs at high zoom get coarser.
    const double omega = angle_between(arc->a, arc->b);
    const double step = std::max(max_step, omega / static_cast<double>(kMaxSubdivisions - 1));
    const std::size_t n = subdivide_arc(arc->a, arc->b, step, pts, kMaxSubdivisions + 1);
    if (n < 2) {
        draw_screen_line(fb, cam, arc->a, arc->b, color, stats);
        return;
    }
    for (std::size_t i = 1; i < n; ++i) {
        draw_screen_line(fb, cam, pts[i - 1], pts[i], color, stats);
    }
}

void draw_graticule(Framebuffer& fb, const Camera& cam, Rgb565 color, RenderStats& stats) {
    // Parallels (not great circles): sampled finely enough that each piece is a short arc.
    for (int lat = -60; lat <= 60; lat += kGraticuleSpacingDeg) {
        Vec3 prev = to_camera_space(cam, to_unit_vector({static_cast<double>(lat), -180.0}));
        for (int lon = -180 + kGraticuleSampleDeg; lon <= 180; lon += kGraticuleSampleDeg) {
            const Vec3 cur = to_camera_space(
                cam, to_unit_vector({static_cast<double>(lat), static_cast<double>(lon)}));
            draw_arc(fb, cam, prev, cur, color, stats);
            prev = cur;
        }
    }
    // Meridians are great circles: one arc per sample, pole to pole.
    for (int lon = -180; lon < 180; lon += kGraticuleSpacingDeg) {
        Vec3 prev = to_camera_space(cam, to_unit_vector({-90.0, static_cast<double>(lon)}));
        for (int lat = -90 + kGraticuleSampleDeg; lat <= 90; lat += kGraticuleSampleDeg) {
            const Vec3 cur = to_camera_space(
                cam, to_unit_vector({static_cast<double>(lat), static_cast<double>(lon)}));
            draw_arc(fb, cam, prev, cur, color, stats);
            prev = cur;
        }
    }
}

void draw_atlas_layer(Framebuffer& fb, const Camera& cam, const Atlas& atlas, AtlasLayer layer,
                      Rgb565 color, RenderStats& stats) {
    AtlasCursor cursor;
    AtlasPolyline line{AtlasLayer::Coastline, 0, nullptr};
    while (atlas.next(cursor, line)) {
        if (line.layer != layer) {
            continue;
        }
        // Each vertex is transformed once and carried to the next segment.
        Vec3 prev = to_camera_space(cam, to_unit_vector(line.point(0)));
        for (std::uint16_t i = 1; i < line.point_count; ++i) {
            const Vec3 cur = to_camera_space(cam, to_unit_vector(line.point(i)));
            draw_arc(fb, cam, prev, cur, color, stats);
            prev = cur;
        }
    }
}

} // namespace

RenderStats render_globe(Framebuffer& fb, const Camera& camera, const Atlas& atlas,
                         const Palette& palette, const Layers& layers) {
    RenderStats stats;
    fb.clear(palette.background);
    fill_disc(fb, camera.center_x, camera.center_y, camera.radius_px, palette.ocean);
    if (layers.graticule) {
        draw_graticule(fb, camera, palette.graticule, stats);
    }
    if (layers.coastlines) {
        draw_atlas_layer(fb, camera, atlas, AtlasLayer::Coastline, palette.coastline, stats);
    }
    if (layers.borders) {
        draw_atlas_layer(fb, camera, atlas, AtlasLayer::Border, palette.border, stats);
    }
    return stats;
}

} // namespace globe
