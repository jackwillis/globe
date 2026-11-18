// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Reuse the unchanged extractor in this TU; our main never calls rendering/reads images.
#define main unused_fixture_export_main
#include "fixtures.cpp"
#undef main

void describe(const Fixture& f) {
    std::printf("{\"features\":%u,\"chains\":%u,\"vertices\":%u,\"detail\":[", f.nf, f.nc, f.np);
    for (unsigned i = 0; i < f.nf; ++i) {
        const auto feature = f.features[i];
        std::printf("%s{\"id\":%u,\"color\":%u,\"chain_count\":%u,\"chains\":[", i ? "," : "",
                    feature.id, feature.color, feature.count);
        for (unsigned j = 0; j < feature.count; ++j) {
            auto c = f.chains[feature.start + j];
            unsigned segments = 0, joins = 0;
            ScreenPoint first{};
            for (unsigned k = 1; k < c.count; ++k) {
                const auto a = f.points[c.start + k - 1], b = f.points[c.start + k];
                if (a.x == b.x && a.y == b.y)
                    continue;
                if (segments)
                    ++joins;
                else
                    first = a;
                ++segments;
            }
            if (c.closed && segments) {
                auto last = f.points[c.start + c.count - 1];
                if (last.x != first.x || last.y != first.y) {
                    ++segments;
                    ++joins;
                }
                ++joins;
            }
            std::printf("%s{\"closed\":%s,\"vertices\":%u,\"supplied_edges\":%u,\"segments\":%u,"
                        "\"joins\":%u}",
                        j ? "," : "", c.closed ? "true" : "false", c.count,
                        c.count ? c.count - 1 : 0, segments, joins);
        }
        std::printf("]}");
    }
    std::printf("]}");
}
int main(int argc, char** argv) {
    if (argc != 3)
        return 2;
    const std::string repo = argv[1], out = argv[2];
    Bytes bytes;
    if (!bytes.load(repo + "/data/atlas.gatl"))
        return 3;
    auto atlas = Atlas::parse(bytes.p.get(), bytes.n);
    if (!atlas)
        return 3;
    auto scene = Scene::create(*atlas);
    if (!scene)
        return 4;
    const LatLon centers[] = {{30, 10}, {62, 8}, {-3, 120}, {48, 15},
                              {0, 179}, {89, 0}, {-89, 0},  {-20, -150}};
    for (unsigned view = 0; view < 8; ++view)
        for (double radius : {220., 440.})
            for (TerrainStyle style : {TerrainStyle::Map, TerrainStyle::Relief}) {
                auto cam = make_camera({centers[view], 0, radius, 640, 480});
                if (!cam)
                    return 5;
                Palette palette = palette_for_style(kDefaultPalette, style);
                if (style != TerrainStyle::Map)
                    palette.border = rgb565(0, 0, 0);
                Fixture f;
                if (!extract(f, *scene, *cam, palette))
                    return 6;
                char stem[80];
                std::snprintf(stem, sizeof stem, "view%u-r%.0f-%s", view, radius,
                              style == TerrainStyle::Map ? "map" : "relief");
                if (!geometry(out + "/" + stem + ".geometry", f))
                    return 7;
                std::printf("{\"fixture\":\"%s\",\"counts\":", stem);
                describe(f);
                std::printf("}\n");
            }
}
