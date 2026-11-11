// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/render.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>

#include "globe/raster.hpp"
#include "globe/terrain.hpp"

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
void draw_arc(Framebuffer& fb, const Camera& cam, Vec3 a, Vec3 b, Rgb565 color, RenderStats& stats,
              double chord_px = kMaxChordPx) {
    ++stats.segments;
    const std::optional<Arc> arc = clip_arc_to_front(a, b);
    if (!arc) {
        return;
    }
    ++stats.segments_visible;
    const double max_step = chord_px / cam.radius_px;
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

// Row-band rejection for the refinement job. A band redraws only what can touch its rows, so
// an arc is rasterized by the one or two bands it crosses instead of by all of them. The tests
// are conservative: they may keep an arc that draws nothing in the band, never drop one that
// would draw a pixel, so the finished frame equals the single-pass frame bit for bit.
//
// Margin: lines cover rows floor(ymin)..floor(ymax); interior points of a subdivided arc are
// normalized with a Newton step good to 1e-5 of the radius.
double band_margin_px(const Camera& cam) {
    return 2.0 + 1.0e-4 * cam.radius_px;
}

bool rows_miss_band(double y_lo, double y_hi, int first, int end) {
    return y_hi < static_cast<double>(first) || y_lo >= static_cast<double>(end);
}

// The projection is 1-Lipschitz, so the screen arc stays within the 3D deviation of the great
// circle from its chord, R * (1 - cos(phi / 2)) <= R * (pi^2 / 32) * |a - b|^2 (no square root).
bool arc_misses_band(const Camera& cam, Vec3 a, Vec3 b, int first, int end) {
    const Vec3 d = a - b;
    const double slack = band_margin_px(cam) + cam.radius_px * 0.31 * dot(d, d);
    const double ya = cam.center_y - cam.radius_px * a.y;
    const double yb = cam.center_y - cam.radius_px * b.y;
    return rows_miss_band(std::min(ya, yb) - slack, std::max(ya, yb) + slack, first, end);
}

// Every vertex and arc of a cap with sin_radius < 1 lies within 2 sin(theta / 2) <= sqrt(2) *
// sin(theta) of the axis in the (1-Lipschitz) projection. Caps of 90 degrees or more never miss.
bool cap_misses_band(const Camera& cam, const PolylineCap& cap, int first, int end) {
    if (!(cap.sin_radius < 1.0)) {
        return false;
    }
    const double yc = cam.center_y - cam.radius_px * dot(cap.axis, cam.up);
    const double slack = band_margin_px(cam) + cam.radius_px * 1.4143 * cap.sin_radius;
    return rows_miss_band(yc - slack, yc + slack, first, end);
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
                    ParallelPoint parallel, MeridianPoint meridian, double chord_px = kMaxChordPx) {
    for (int p = 0; p < kParallels; ++p) {
        Vec3 prev = to_camera_space(cam, parallel(p, 0));
        for (int s = 1; s < kParallelSamples; ++s) {
            const Vec3 cur = to_camera_space(cam, parallel(p, s));
            draw_arc(fb, cam, prev, cur, color, stats, chord_px);
            prev = cur;
        }
    }
    for (int m = 0; m < kMeridians; ++m) {
        Vec3 prev = to_camera_space(cam, meridian(m, 0));
        for (int s = 1; s < kMeridianSamples; ++s) {
            const Vec3 cur = to_camera_space(cam, meridian(m, s));
            draw_arc(fb, cam, prev, cur, color, stats, chord_px);
            prev = cur;
        }
    }
}

// Draws one layer of the atlas; `vertex(line, first_point, i)` supplies each world-space vertex.
// Each vertex is transformed once and carried to the next segment.
template <typename VertexAt>
void draw_atlas_layer(Framebuffer& fb, const Camera& cam, const Atlas& atlas, AtlasLayer layer,
                      Rgb565 color, RenderStats& stats, VertexAt vertex,
                      const PolylineCap* caps = nullptr, double chord_px = kMaxChordPx) {
    AtlasCursor cursor;
    AtlasPolyline line{AtlasLayer::Coastline, 0, nullptr};
    for (;;) {
        const std::uint32_t index = cursor.index;
        const std::uint32_t first = cursor.first_point;
        if (!atlas.next(cursor, line)) {
            break;
        }
        if (line.layer != layer) {
            continue;
        }
        // A cap wholly behind the horizon cannot contribute pixels. Radius is cached as its
        // sine, so the frame loop needs only a dot product. Keep the tangent and a rounding
        // margin; caps >= a hemisphere have sin_radius=1 and can never pass this test.
        if (caps != nullptr &&
            dot(caps[index].axis, cam.forward) < -caps[index].sin_radius - 1e-12) {
            ++stats.polylines_culled;
            continue;
        }
        Vec3 prev = to_camera_space(cam, vertex(line, first, 0));
        for (std::uint16_t i = 1; i < line.point_count; ++i) {
            const Vec3 cur = to_camera_space(cam, vertex(line, first, i));
            draw_arc(fb, cam, prev, cur, color, stats, chord_px);
            prev = cur;
        }
    }
}

} // namespace

std::optional<Scene> Scene::create(const Atlas& atlas, MemoryLedger* ledger) {
    const std::size_t total = static_cast<std::size_t>(atlas.point_count()) + kGraticulePoints;
    MemoryCharge storage_charge, caps_charge;
    std::unique_ptr<Vec3[]> storage(new (std::nothrow) Vec3[total]);
    if (!storage) {
        if (ledger != nullptr)
            ledger->allocation_failed(MemoryCategory::Scene);
        return std::nullopt;
    }
    if (ledger != nullptr)
        storage_charge = MemoryCharge(*ledger, MemoryCategory::Scene, total * sizeof(Vec3));
    std::unique_ptr<PolylineCap[]> caps(new (std::nothrow) PolylineCap[atlas.polyline_count()]);
    if (!caps) {
        if (ledger != nullptr)
            ledger->allocation_failed(MemoryCategory::Scene);
        return std::nullopt;
    }
    if (ledger != nullptr)
        caps_charge =
            MemoryCharge(*ledger, MemoryCategory::Scene,
                         static_cast<std::size_t>(atlas.polyline_count()) * sizeof(PolylineCap));
    Vec3* out = storage.get();
    AtlasCursor cursor;
    AtlasPolyline line{AtlasLayer::Coastline, 0, nullptr};
    while (atlas.next(cursor, line)) {
        Vec3* first = out;
        Vec3 sum{};
        for (std::uint16_t i = 0; i < line.point_count; ++i) {
            *out = to_unit_vector(line.point(i));
            sum = sum + *out++;
        }
        PolylineCap cap{{0.0, 0.0, 0.0}, 1.0};
        const double sum2 = dot(sum, sum);
        if (sum2 > 1e-24) {
            cap.axis = sum * (1.0 / std::sqrt(sum2));
            double min_cos = 1.0;
            for (std::uint16_t i = 0; i < line.point_count; ++i) {
                min_cos = std::min(min_cos, dot(cap.axis, first[i]));
            }
            if (min_cos > 0.0) {
                cap.sin_radius = std::sqrt(std::max(0.0, 1.0 - min_cos * min_cos));
            }
        }
        caps[cursor.index - 1] = cap;
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
    return Scene(atlas, std::move(storage), std::move(caps), std::move(storage_charge),
                 std::move(caps_charge));
}

Scene& Scene::operator=(Scene&& other) noexcept {
    if (this != &other) {
        // Free replaced storage before releasing its charges, including cross-ledger moves.
        storage_.reset();
        caps_.reset();
        storage_charge_ = std::move(other.storage_charge_);
        caps_charge_ = std::move(other.caps_charge_);
        atlas_ = other.atlas_;
        storage_ = std::move(other.storage_);
        caps_ = std::move(other.caps_);
    }
    return *this;
}

std::size_t Scene::bytes() const {
    return (static_cast<std::size_t>(atlas_.point_count()) + kGraticulePoints) * sizeof(Vec3) +
           static_cast<std::size_t>(atlas_.polyline_count()) * sizeof(PolylineCap);
}

Palette palette_for_style(Palette palette, TerrainStyle style) {
    if (style == TerrainStyle::Map) {
        palette.ocean = kMapOcean;
        palette.coastline = rgb565(80, 118, 104);
        palette.border = rgb565(158, 148, 132);
        palette.graticule = rgb565(120, 168, 206);
    }
    return palette;
}

RenderStats render_globe(Framebuffer& fb, const Camera& camera, const Scene& scene,
                         const Palette& requested_palette, const Layers& layers,
                         bool cull_polylines, RenderQuality quality, const TerrainScene* base) {
    auto palette = palette_for_style(requested_palette, quality.terrain_style);
    if (quality.lines_only && quality.terrain_style != TerrainStyle::Map)
        palette.border = rgb565(0, 0, 0); // as a terrain job draws it
    const double chord_px = std::isfinite(quality.max_chord_px)
                                ? std::clamp(quality.max_chord_px, 0.25, 16.0)
                                : kMaxChordPx;
    RenderStats stats;
    if (!quality.lines_only) {
        fb.clear(palette.background);
        fill_disc(fb, camera.center_x, camera.center_y, camera.radius_px, palette.ocean);
        if (base && quality.gray_base)
            base->draw_base(fb, camera);
    }
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
            },
            chord_px);
    }
    const Vec3* vertices = scene.vertices();
    const auto vertex = [vertices](const AtlasPolyline&, std::uint32_t first, std::uint16_t i) {
        return vertices[first + i];
    };
    if (layers.coastlines) {
        draw_atlas_layer(fb, camera, scene.atlas(), AtlasLayer::Coastline, palette.coastline, stats,
                         vertex, cull_polylines ? scene.caps() : nullptr, chord_px);
    }
    if (layers.borders) {
        draw_atlas_layer(fb, camera, scene.atlas(), AtlasLayer::Border, palette.border, stats,
                         vertex, cull_polylines ? scene.caps() : nullptr, chord_px);
    }
    return stats;
}

RenderStats render_globe(Framebuffer& fb, const Camera& camera, const Atlas& atlas,
                         const Palette& palette, const Layers& layers,
                         const TerrainScene* /*base*/) {
    // The reference path never draws the gray base: plain vectors, matching the default quality.
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

// Marks "no line here" in the line layer; no palette color equals it.
constexpr Rgb565 kLineLayerKey = 0x0001;

RefinementJob::RefinementJob(const Camera& camera, const Scene& scene, const Palette& palette,
                             Layers layers, RenderQuality quality, std::uint64_t generation,
                             TerrainScene* terrain)
    : camera_(camera), scene_(&scene), palette_(palette), layers_(layers), quality_(quality),
      generation_(generation) {
    palette_ = palette_for_style(palette_, quality_.terrain_style);
    if (quality_.terrain && terrain != nullptr) {
        terrain_ = terrain;
        terrain_token_ = terrain_->claim();
        phase_ = Phase::ProjectCorners;
        if (quality_.terrain_style != TerrainStyle::Map)
            palette_.border = rgb565(0, 0, 0);
    }
    if (terrain_ != nullptr && quality_.lines_first) {
        line_layer_ = Framebuffer::create(camera_.width, camera_.height);
        if (line_layer_.has_value()) {
            line_list_capacity_ =
                quality_.line_list_limit != 0
                    ? quality_.line_list_limit
                    : std::max<std::size_t>(256, static_cast<std::size_t>(camera_.width) *
                                                     camera_.height / 16);
            line_list_.reset(new (std::nothrow) std::uint32_t[line_list_capacity_]);
            if (line_list_ == nullptr)
                line_list_capacity_ = 0; // the composite scans the whole layer instead
            phase_ = Phase::Clear; // lines first: clear, draw the layer and show it before any
                                   // terrain set-up
        }
    }
    if (terrain_ != nullptr)
        plan_ = make_pass_plan(camera_, quality_.focus);
    quality_.max_chord_px = std::isfinite(quality_.max_chord_px)
                                ? std::clamp(quality_.max_chord_px, 0.25, 16.0)
                                : kMaxChordPx;
}

double RefinementJob::progress() const {
    if (!active_)
        return 1.0;
    if (terrain_ == nullptr)
        return std::clamp(static_cast<double>(band_first_) / camera_.height, 0.0, 1.0);
    const auto fraction = [](std::size_t done, std::size_t total) {
        return total == 0 ? 0.0 : static_cast<double>(done) / static_cast<double>(total);
    };
    // With the line layer the lines come first, then the terrain set-up, then the passes.
    const bool lines_first = line_layer_.has_value();
    if (lines_first && (phase_ == Phase::IndexLineLayer ||
                        (phase_ == Phase::Composite && after_stamp_ == Phase::ProjectCorners)))
        return 0.05; // listing and showing the line layer
    switch (phase_) {
    case Phase::ProjectCorners:
        return lines_first ? 0.06 + 0.02 * fraction(prepare_index_, terrain_->corner_count())
                           : 0.03 * fraction(prepare_index_, terrain_->corner_count());
    case Phase::BoundCells:
        return lines_first ? 0.08 + 0.02 * fraction(prepare_index_, terrain_->cell_count())
                           : 0.03 + 0.03 * fraction(prepare_index_, terrain_->cell_count());
    case Phase::Clear:
    case Phase::ClearLineLayer:
    case Phase::Parallels:
    case Phase::Meridians:
    case Phase::Coastlines:
    case Phase::Borders:
        return lines_first ? 0.03 : 0.08; // clearing and the line layer: short
    case Phase::RestoreUncovered:
        return 0.98;
    default: {
        // The ring passes and the limb pass: pass_ completed plus the sweep through the cells.
        const unsigned passes = kRefinePasses + 1;
        const bool limb = pass_ >= kRefinePasses;
        const double within =
            limb || !terrain_->has_lists()
                ? fraction(terrain_cell_, terrain_->cell_count())
                : fraction(list_pos_, std::max<std::size_t>(1, terrain_->pass_count(pass_)));
        const double done = (static_cast<double>(pass_) + within) / passes;
        return std::clamp(0.1 + 0.88 * done, 0.0, 0.98);
    }
    }
}

// Cells the limb pass looks at per work unit while it searches for the next clipped cell.
constexpr std::size_t kLimbScanCells = 256;

RenderStep RefinementJob::step(Framebuffer& fb, std::uint64_t generation, unsigned max_work_units) {
    RenderStep result;
    if (!active_ || generation != generation_ ||
        (terrain_ != nullptr && terrain_->token() != terrain_token_) ||
        fb.width() != camera_.width || fb.height() != camera_.height) {
        active_ = false;
        return result;
    }
    if (max_work_units == 0)
        return result;
    // With terrain the whole frame is one focus-ordered job; without it, top-down 16-row bands.
    result.first_row = terrain_ != nullptr ? 0 : band_first_;
    result.end_row =
        terrain_ != nullptr ? fb.height() : std::min(band_first_ + kRefineBandRows, fb.height());
    const int saved_first = fb.first_row();
    const int saved_end = fb.end_row();
    fb.clip_rows(result.first_row, result.end_row);
    unsigned fills = 0;
    while (result.work_units < max_work_units && !result.band_ready) {
        ++result.work_units;
        if (phase_ == Phase::ProjectCorners) {
            terrain_->project_corner(prepare_index_++, camera_);
            if (prepare_index_ == terrain_->corner_count()) {
                phase_ = Phase::BoundCells;
                prepare_index_ = 0;
                if (terrain_->has_lists())
                    terrain_->begin_lists();
            }
        } else if (phase_ == Phase::BoundCells) {
            terrain_->bound_cell(prepare_index_, fb.height(), kRefineBandRows);
            terrain_->assign_pass(prepare_index_, plan_);
            ++prepare_index_;
            if (prepare_index_ == terrain_->cell_count()) {
                prepare_index_ = 0;
                if (terrain_->has_lists()) {
                    terrain_->finish_lists();
                    phase_ = Phase::BuildPassLists;
                } else {
                    phase_ = line_layer_.has_value() ? Phase::RingPass : Phase::Clear;
                }
            }
        } else if (phase_ == Phase::BuildPassLists) {
            // Group the cells by ring (index order inside each), one cell per work unit like the
            // set-up phases, so the passes below visit only the cells they draw.
            terrain_->place_cell(prepare_index_++);
            if (prepare_index_ == terrain_->cell_count()) {
                phase_ = line_layer_.has_value() ? Phase::RingPass : Phase::Clear;
                prepare_index_ = 0;
            }
        } else if (phase_ == Phase::RingPass) {
            // Pass k paints the cells of ring k. A cell clipped at the horizon is a flat polygon
            // and overlaps its shaded neighbors by a pixel along shared edges, so after the rings
            // one more pass redraws the clipped cells in index order: the winner of every pixel is
            // then independent of the ring order (checked on 600 random views, tests/test_terrain).
            const bool last = pass_ == kRefinePasses; // the limb pass
            // With lists the ring passes walk their own cell list, otherwise (a fine grid) they
            // test every cell. The limb pass walks the clipped cells, looking at a bounded number
            // of cells per work unit.
            const bool lists = terrain_->has_lists();
            std::size_t cell = terrain_cell_;
            bool draw;
            if (last) {
                terrain_cell_ = terrain_->skip_to_limb(terrain_cell_, kLimbScanCells);
                cell = terrain_cell_;
                draw = cell < terrain_->cell_count() && terrain_->is_limb(cell);
            } else if (lists) {
                draw = list_pos_ < terrain_->pass_count(pass_);
                if (draw)
                    cell = terrain_->pass_cell(pass_, list_pos_);
            } else {
                draw = terrain_->cell_pass(cell) == pass_;
            }
            if (draw) {
                const auto bands = terrain_->cell_bands(cell);
                if (cell_band_ < 0)
                    cell_band_ = bands.first;
                fb.clip_rows(cell_band_ * kRefineBandRows,
                             std::min(fb.height(), (cell_band_ + 1) * kRefineBandRows));
                if (quality_.preserve_bitmap)
                    fb.coverage(Framebuffer::Coverage::Mark);
                terrain_->draw_cell(fb, cell, cell_band_, kRefineBandRows, quality_.terrain_style,
                                    quality_.smooth_terrain);
                fb.coverage(Framebuffer::Coverage::Off);
                fb.clip_rows(0, fb.height());
                if (++cell_band_ == bands.end) {
                    cell_band_ = -1;
                    if (lists && !last)
                        ++list_pos_;
                    else
                        ++terrain_cell_;
                }
                ++stats_.terrain_slices;
                ++fills;
            } else if (!lists && !last) {
                ++terrain_cell_;
            }
            if (lists && !last ? list_pos_ >= terrain_->pass_count(pass_)
                               : terrain_cell_ >= terrain_->cell_count()) {
                terrain_cell_ = 0;
                list_pos_ = 0;
                ++pass_;
                result.band_ready = !line_layer_.has_value(); // else shown after the stamp
                if (line_layer_.has_value()) {
                    row_ = 0;
                    after_stamp_ = last ? Phase::Done : Phase::RingPass;
                    phase_ = last && quality_.preserve_bitmap ? Phase::RestoreUncovered
                                                              : Phase::Composite;
                } else if (last) {
                    row_ = 0;
                    phase_ = quality_.preserve_bitmap ? Phase::RestoreUncovered
                             : quality_.terrain_only  ? Phase::Done
                                                      : Phase::Parallels;
                }
            }
            // Bound a fill continuation by four 16-row cell slices as well as work units. This
            // changes scheduling, not smoothing or rasterization.
            if (fills == 4)
                break;
        } else if (phase_ == Phase::RestoreUncovered) {
            // Only pixels untouched by the finest canonical terrain pass are restored to
            // background/ocean. This removes coarse limb chords and islands without erasing detail.
            fb.clip_rows(row_, row_ + 1);
            fb.coverage(Framebuffer::Coverage::Uncovered);
            fb.fill_span(row_, 0, fb.width() - 1, palette_.background);
            fill_disc(fb, camera_.center_x, camera_.center_y, camera_.radius_px, palette_.ocean);
            fb.coverage(Framebuffer::Coverage::Off);
            fb.clip_rows(result.first_row, result.end_row);
            if (++row_ == fb.height()) {
                row_ = 0;
                phase_ = line_layer_.has_value() ? Phase::Composite
                         : quality_.terrain_only ? Phase::Done
                                                 : Phase::Parallels;
                result.band_ready = true;
            }
        } else if (phase_ == Phase::IndexLineLayer) {
            // One row of the finished line layer: note where its lines are.
            if (line_list_ok_) {
                const auto* lines =
                    line_layer_->data() + static_cast<std::size_t>(row_) * fb.width();
                for (int x = 0; x < fb.width(); ++x) {
                    if (lines[x] == kLineLayerKey)
                        continue;
                    if (line_list_count_ == line_list_capacity_) {
                        line_list_ok_ = false; // more than the list holds: scan instead
                        break;
                    }
                    line_list_[line_list_count_++] =
                        (static_cast<std::uint32_t>(row_) << 16) | static_cast<std::uint32_t>(x);
                }
            }
            if (!line_list_ok_ || ++row_ == fb.height()) {
                row_ = 0;
                stamp_pos_ = 0;
                phase_ = Phase::Composite;
            }
        } else if (phase_ == Phase::Composite) {
            if (stamp_pos_ == 0 && after_stamp_ == Phase::Done && snapshot_ != nullptr &&
                !snapshot_taken_ && snapshot_->width() == fb.width() &&
                snapshot_->height() == fb.height()) {
                std::memcpy(snapshot_->data(), fb.data(), fb.byte_size());
                snapshot_taken_ = true; // the terrain alone: the lines are stamped next
            }
            // Composite the line layer over the frame (a pass may have painted over the lines).
            bool done = false;
            if (line_list_ok_) {
                // A bounded run of the listed line pixels.
                const std::size_t end = std::min(stamp_pos_ + 256, line_list_count_);
                const auto* lines = line_layer_->data();
                for (std::size_t i = stamp_pos_; i < end; ++i) {
                    const auto y = static_cast<std::size_t>(line_list_[i] >> 16);
                    const auto x = static_cast<std::size_t>(line_list_[i] & 0xffffU);
                    fb.data()[y * static_cast<std::size_t>(fb.stride()) + x] =
                        lines[y * static_cast<std::size_t>(fb.width()) + x];
                }
                stamp_pos_ = end;
                done = stamp_pos_ >= line_list_count_;
            } else {
                // Too many line pixels for the list: scan one row of the layer.
                const auto* lines =
                    line_layer_->data() + stamp_pos_ * static_cast<std::size_t>(fb.width());
                auto* pixels = fb.data() + stamp_pos_ * static_cast<std::size_t>(fb.stride());
                for (int x = 0; x < fb.width(); ++x)
                    if (lines[x] != kLineLayerKey)
                        pixels[x] = lines[x];
                done = ++stamp_pos_ == static_cast<std::size_t>(fb.height());
            }
            if (done) {
                stamp_pos_ = 0;
                phase_ = after_stamp_;
                result.band_ready = true;
            }
        } else if (phase_ == Phase::ClearLineLayer) {
            line_layer_->fill_span(row_, 0, fb.width() - 1, kLineLayerKey);
            if (++row_ == fb.height()) {
                row_ = 0;
                phase_ = Phase::Parallels;
                lines_drawing_ = true;
            }
        } else if (phase_ == Phase::Clear) {
            fb.clip_rows(row_, row_ + 1);
            if (quality_.preserve_bitmap && terrain_ != nullptr) {
                fb.reset_coverage_row(row_);
            } else {
                fb.fill_span(row_, 0, fb.width() - 1, palette_.background);
                fill_disc(fb, camera_.center_x, camera_.center_y, camera_.radius_px,
                          palette_.ocean);
            }
            fb.clip_rows(result.first_row, result.end_row);
            if (++row_ == result.end_row) {
                phase_ = terrain_ != nullptr
                             ? (line_layer_.has_value() ? Phase::ClearLineLayer : Phase::RingPass)
                             : Phase::Parallels;
                terrain_cell_ = 0;
                list_pos_ = 0;
                row_ = 0;
            }
        } else if (phase_ == Phase::Parallels || phase_ == Phase::Meridians) {
            const bool parallels = phase_ == Phase::Parallels;
            const int samples = parallels ? kParallelSamples : kMeridianSamples;
            const int lines = parallels ? kParallels : kMeridians;
            if (!layers_.graticule || grid_segment_ == lines * (samples - 1)) {
                phase_ = next(phase_);
                grid_segment_ = 0;
                continue;
            }
            const int line = grid_segment_ / (samples - 1);
            const int point = grid_segment_ % (samples - 1);
            const Vec3* grid =
                scene_->graticule() + (parallels ? 0 : kParallels * kParallelSamples);
            const int first = line * samples + point;
            const Vec3 a = to_camera_space(camera_, grid[first]);
            const Vec3 b = to_camera_space(camera_, grid[first + 1]);
            if (arc_misses_band(camera_, a, b, result.first_row, result.end_row)) {
                ++stats_.segments_skipped;
            } else {
                draw_arc(lines_drawing_ ? *line_layer_ : fb, camera_, a, b, palette_.graticule,
                         stats_, quality_.max_chord_px);
            }
            ++grid_segment_;
        } else if (phase_ == Phase::Coastlines || phase_ == Phase::Borders) {
            const bool coast = phase_ == Phase::Coastlines;
            if ((coast && !layers_.coastlines) || (!coast && !layers_.borders)) {
                phase_ = next(phase_);
                cursor_ = {};
                segment_ = 0;
                continue;
            }
            if (segment_ == 0) {
                const auto index = cursor_.index;
                line_first_ = cursor_.first_point;
                if (!scene_->atlas().next(cursor_, line_)) {
                    phase_ = next(phase_);
                    cursor_ = {};
                    continue;
                }
                if (line_.layer != (coast ? AtlasLayer::Coastline : AtlasLayer::Border))
                    continue;
                const auto& cap = scene_->caps()[index];
                if (dot(cap.axis, camera_.forward) < -cap.sin_radius - 1e-12) {
                    ++stats_.polylines_culled;
                    continue;
                }
                if (cap_misses_band(camera_, cap, result.first_row, result.end_row)) {
                    ++stats_.polylines_skipped;
                    continue;
                }
                segment_ = 1;
            } else {
                const Vec3* points = scene_->vertices() + line_first_;
                const Vec3 a = to_camera_space(camera_, points[segment_ - 1]);
                const Vec3 b = to_camera_space(camera_, points[segment_]);
                if (arc_misses_band(camera_, a, b, result.first_row, result.end_row)) {
                    ++stats_.segments_skipped;
                } else {
                    draw_arc(lines_drawing_ ? *line_layer_ : fb, camera_, a, b,
                             coast ? palette_.coastline : palette_.border, stats_,
                             quality_.max_chord_px);
                }
                if (++segment_ == line_.point_count)
                    segment_ = 0;
            }
        } else if (lines_drawing_) {
            lines_drawing_ = false; // the layer is complete: list its pixels, show it, then terrain
            row_ = 0;
            line_list_count_ = 0;
            line_list_ok_ = line_list_ != nullptr;
            after_stamp_ = Phase::ProjectCorners; // then project the terrain corners
            phase_ = Phase::IndexLineLayer;
        } else {
            result.band_ready = true;
            if (terrain_ != nullptr) {
                active_ = false;
            } else {
                band_first_ = result.end_row;
                row_ = band_first_;
                phase_ = Phase::Clear;
                active_ = band_first_ < fb.height();
            }
        }
    }
    fb.clip_rows(saved_first, saved_end);
    return result;
}

} // namespace globe
