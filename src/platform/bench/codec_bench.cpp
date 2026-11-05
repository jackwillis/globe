// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Times the GL-29 prototype decoders: the compressed grid decoder (grid_codec) on .ggrd files and
// the delta-varint point decoder (point_codec) on an atlas plus its .dvar stream. Prints the median
// and minimum over the repeats and a CRC-32 of the decoded data so host and target can be compared.
//
//   codec-bench [--repeat N] grid FILE.ggrd...
//   codec-bench [--repeat N] points ATLAS.gatl FILE.dvar
#include "clock.hpp"
#include "globe/atlas.hpp"
#include "globe/crc32.hpp"
#include "globe/grid_codec.hpp"
#include "globe/point_codec.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace globe;

namespace {

bool read_file(const char* path, std::vector<std::uint8_t>& data) {
    std::FILE* f = std::fopen(path, "rb");
    if (f == nullptr) {
        std::fprintf(stderr, "codec-bench: cannot open %s\n", path);
        return false;
    }
    std::uint8_t buffer[8192];
    std::size_t n;
    data.clear();
    while ((n = std::fread(buffer, 1, sizeof buffer, f)) > 0) {
        data.insert(data.end(), buffer, buffer + n);
    }
    std::fclose(f);
    return true;
}

struct Summary {
    double median_ms;
    double min_ms;
};

Summary summarize(std::vector<std::uint64_t>& ns) {
    std::sort(ns.begin(), ns.end());
    return {static_cast<double>(ns[ns.size() / 2]) / 1.0e6,
            static_cast<double>(ns.front()) / 1.0e6};
}

int bench_grid(const char* path, int repeat) {
    std::vector<std::uint8_t> stream;
    if (!read_file(path, stream)) {
        return 1;
    }
    GridCodecInfo info;
    if (grid_codec_info(stream.data(), stream.size(), info) != GridCodecError::Ok) {
        std::fprintf(stderr, "codec-bench: %s is not a valid grid\n", path);
        return 1;
    }
    const std::size_t cells = static_cast<std::size_t>(info.width) * info.height;
    std::vector<std::uint8_t> out(cells);
    std::vector<std::uint64_t> ns;
    for (int i = 0; i < repeat; ++i) {
        const std::uint64_t t0 = monotonic_ns();
        const GridCodecError e =
            grid_codec_decode(stream.data(), stream.size(), out.data(), out.size());
        const std::uint64_t t1 = monotonic_ns();
        if (e != GridCodecError::Ok) {
            std::fprintf(stderr, "codec-bench: decode failed (%d)\n", static_cast<int>(e));
            return 1;
        }
        ns.push_back(t1 - t0);
    }
    const Summary s = summarize(ns);
    std::printf("grid %s width=%u height=%u cells=%lu stored=%lu median_ms=%.3f min_ms=%.3f "
                "ns_per_cell=%.1f crc=%08x\n",
                path, static_cast<unsigned>(info.width), static_cast<unsigned>(info.height),
                static_cast<unsigned long>(cells), static_cast<unsigned long>(stream.size()),
                s.median_ms, s.min_ms, s.median_ms * 1.0e6 / static_cast<double>(cells),
                static_cast<unsigned>(crc32(out.data(), out.size())));
    return 0;
}

int bench_points(const char* atlas_path, const char* stream_path, int repeat) {
    std::vector<std::uint8_t> gatl;
    std::vector<std::uint8_t> stream;
    if (!read_file(atlas_path, gatl) || !read_file(stream_path, stream)) {
        return 1;
    }
    const auto atlas = Atlas::parse(gatl.data(), gatl.size());
    if (!atlas) {
        std::fprintf(stderr, "codec-bench: bad atlas\n");
        return 1;
    }
    std::vector<std::uint16_t> out(std::size_t{2} * 65536);
    std::vector<std::uint64_t> ns;
    std::uint32_t crc = 0;
    for (int i = 0; i < repeat; ++i) {
        AtlasCursor cursor;
        AtlasPolyline line;
        std::size_t pos = 0;
        std::uint32_t running = 0;
        const std::uint64_t t0 = monotonic_ns();
        while (atlas->next(cursor, line)) {
            std::size_t used = 0;
            if (decode_delta_points(stream.data() + pos, stream.size() - pos, &used, out.data(),
                                    line.point_count) != PointCodecError::Ok) {
                std::fprintf(stderr, "codec-bench: point decode failed\n");
                return 1;
            }
            pos += used;
            // Fold the output in so the work cannot be optimized away; checked after timing.
            const std::size_t last = std::size_t{2} * (line.point_count - 1U);
            running += out[last] + out[last + 1U];
        }
        const std::uint64_t t1 = monotonic_ns();
        ns.push_back(t1 - t0);
        crc = running;
    }
    const Summary s = summarize(ns);
    std::printf("points %s points=%lu stored=%lu median_ms=%.3f min_ms=%.3f ns_per_point=%.1f "
                "tailsum=%08x\n",
                stream_path, static_cast<unsigned long>(atlas->point_count()),
                static_cast<unsigned long>(stream.size()), s.median_ms, s.min_ms,
                s.median_ms * 1.0e6 / static_cast<double>(atlas->point_count()),
                static_cast<unsigned>(crc));
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    int repeat = 9;
    int i = 1;
    if (i + 1 < argc && std::strcmp(argv[i], "--repeat") == 0) {
        repeat = static_cast<int>(std::strtol(argv[i + 1], nullptr, 10));
        i += 2;
    }
    if (repeat < 1 || i >= argc) {
        std::fprintf(stderr, "usage: codec-bench [--repeat N] grid FILE... | points ATLAS DVAR\n");
        return 2;
    }
    if (std::strcmp(argv[i], "grid") == 0) {
        for (int k = i + 1; k < argc; ++k) {
            if (bench_grid(argv[k], repeat) != 0) {
                return 1;
            }
        }
        return 0;
    }
    if (std::strcmp(argv[i], "points") == 0 && i + 2 < argc) {
        return bench_points(argv[i + 1], argv[i + 2], repeat);
    }
    std::fprintf(stderr, "usage: codec-bench [--repeat N] grid FILE... | points ATLAS DVAR\n");
    return 2;
}
