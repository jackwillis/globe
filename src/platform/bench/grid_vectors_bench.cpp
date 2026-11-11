// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "clock.hpp"
#include "globe/geometry.hpp"
#include "terrain_world.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
using namespace globe;
struct Grid {
    unsigned w, h;
    double west, east, south, north;
};
void reference(Vec3* out, const Grid& g) {
    for (unsigned y = 0; y <= g.h; ++y)
        for (unsigned x = 0; x <= g.w; ++x) {
            double lat = g.south + (g.north - g.south) * y / g.h;
            double lon = g.west + (g.east - g.west) * x / g.w;
            out[std::size_t(y) * (g.w + 1) + x] = to_unit_vector({lat, lon});
        }
}
void candidate(Vec3* out, const Grid& g) {
    detail::initialize_terrain_world(out, g.w, g.h, g.west, g.east, g.south, g.north);
}
std::uint64_t ns() {
    return monotonic_ns();
}
std::uint64_t hash(const std::vector<Vec3>& v) {
    std::uint64_t result = 14695981039346656037ULL;
    for (const auto& p : v)
        for (double d : {p.x, p.y, p.z}) {
            std::uint64_t b;
            std::memcpy(&b, &d, 8);
            result = (result ^ b) * 1099511628211ULL;
        }
    return result;
}
int main() {
    for (const Grid g :
         {Grid{1, 1, -180, 180, -90, 90}, Grid{12, 6, -180, 180, -90, 90},
          Grid{241, 137, 170, 180, -89.99, 71.123}, Grid{37, 23, -178.123, 34.567, -45.321, 90},
          Grid{360, 180, -180, 180, -90, 90}, Grid{720, 360, -180, 180, -90, 90}}) {
        std::vector<Vec3> a(std::size_t(g.w + 1) * (g.h + 1)), b(a.size());
        reference(a.data(), g);
        candidate(b.data(), g);
        if (std::memcmp(a.data(), b.data(), a.size() * sizeof(Vec3)) != 0) {
            std::fprintf(stderr, "mismatch %ux%u\n", g.w, g.h);
            return 1;
        }
    }
    std::printf("{\"compiler\":\"%s\",\"repeat\":9,\"warmup\":1,\"allocation\":\"two preallocated "
                "output arrays; candidate uses its first row as scratch\",\"cases\":[",
                __VERSION__);
    bool comma = false;
    for (const Grid g : {Grid{360, 180, -180, 180, -90, 90}, Grid{720, 360, -180, 180, -90, 90}}) {
        std::vector<Vec3> a(std::size_t(g.w + 1) * (g.h + 1)), b(a.size());
        std::vector<double> old_times, new_times;
        old_times.reserve(9);
        new_times.reserve(9);
        for (int i = -1; i < 9; ++i) {
            const auto a0 = ns();
            reference(a.data(), g);
            const auto a1 = ns();
            candidate(b.data(), g);
            const auto a2 = ns();
            if (i >= 0) {
                old_times.push_back(static_cast<double>(a1 - a0) / 1e6);
                new_times.push_back(static_cast<double>(a2 - a1) / 1e6);
            }
        }
        const auto h = hash(a);
        if (h != hash(b))
            return 1;
        std::printf("%s{\"grid\":\"%ux%u\",\"hash\":\"%016llx\",\"reference_ms\":[",
                    comma ? "," : "", g.w, g.h, static_cast<unsigned long long>(h));
        for (std::size_t i = 0; i < old_times.size(); ++i)
            std::printf("%s%.5f", i ? "," : "", old_times[i]);
        std::printf("],\"candidate_ms\":[");
        for (std::size_t i = 0; i < new_times.size(); ++i)
            std::printf("%s%.5f", i ? "," : "", new_times[i]);
        std::printf("]}");
        comma = true;
    }
    std::printf("]}\n");
    return 0;
}
