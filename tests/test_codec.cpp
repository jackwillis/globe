// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// GL-29 codecs: the grid decoder against vectors written by tools/grid-codec/grid_codec.py, the
// delta-varint point decoder against the real atlas, and hostile-input behavior of both.
#include "check.hpp"
#include "globe/atlas.hpp"
#include "globe/bytes.hpp"
#include "globe/crc32.hpp"
#include "globe/grid_codec.hpp"
#include "globe/point_codec.hpp"

#include <cstdio>
#include <string>
#include <vector>

using namespace globe;
using Bytes = std::vector<std::uint8_t>;

namespace {

std::string root = ".";

Bytes read_file(const std::string& relative) {
    Bytes data;
    std::FILE* f = std::fopen((root + "/" + relative).c_str(), "rb");
    if (f == nullptr) {
        std::printf("cannot open %s\n", relative.c_str());
        ++test::failures();
        return data;
    }
    std::uint8_t buffer[4096];
    std::size_t n;
    while ((n = std::fread(buffer, 1, sizeof buffer, f)) > 0) {
        data.insert(data.end(), buffer, buffer + n);
    }
    std::fclose(f);
    return data;
}

GridCodecError decode(const Bytes& stream, Bytes& cells, GridCodecInfo* info = nullptr) {
    GridCodecInfo local;
    const GridCodecError probe = grid_codec_info(stream.data(), stream.size(), local);
    cells.assign(probe == GridCodecError::Ok ? static_cast<std::size_t>(local.width) * local.height
                                             : 0,
                 0xAA);
    return grid_codec_decode(stream.data(), stream.size(), cells.data(), cells.size(), info);
}

// Rewrites the payload-size and payload-CRC fields so a hand-modified payload reaches the range
// decoder instead of being stopped by the container checks.
void refresh(Bytes& stream) {
    store_le32(stream.data() + 20,
               static_cast<std::uint32_t>(stream.size() - kGridCodecHeaderBytes));
    store_le32(stream.data() + 24,
               crc32(stream.data() + kGridCodecHeaderBytes, stream.size() - kGridCodecHeaderBytes));
}

void check_vector(const char* name, const Bytes& expected) {
    const Bytes stream = read_file(std::string("tests/data/") + name);
    Bytes cells;
    GridCodecInfo info;
    CHECK(decode(stream, cells, &info) == GridCodecError::Ok);
    CHECK(cells == expected);
    CHECK(static_cast<std::size_t>(info.width) * info.height == expected.size());
}

} // namespace

int main(int argc, char** argv) {
    if (argc > 1) {
        root = argv[1];
    }

    // ---- grid: the real one degree grid ------------------------------------------------------
    const Bytes gelv = read_file("data/elevation-1deg.gelv");
    CHECK(gelv.size() == 552 + 360 * 180);
    const Bytes real_cells(gelv.begin() + 552, gelv.end());
    {
        const Bytes stream = read_file("tests/data/grid-1deg.ggrd");
        Bytes cells;
        GridCodecInfo info;
        CHECK(decode(stream, cells, &info) == GridCodecError::Ok);
        CHECK(info.width == 360 && info.height == 180);
        CHECK(cells == real_cells);
        // It really is smaller (the experiment measured 29,661 payload bytes).
        CHECK(stream.size() < real_cells.size() / 2);
    }

    // ---- grid: small and degenerate shapes ---------------------------------------------------
    check_vector("grid-noise.ggrd", read_file("tests/data/grid-noise.cells"));
    check_vector("grid-1x1.ggrd", Bytes{7});
    {
        Bytes row(50);
        for (std::size_t i = 0; i < row.size(); ++i) {
            row[i] = static_cast<std::uint8_t>(i);
        }
        check_vector("grid-row.ggrd", row);
        Bytes col(40);
        for (std::size_t i = 0; i < col.size(); ++i) {
            col[i] = static_cast<std::uint8_t>((i * 7) & 255);
        }
        check_vector("grid-col.ggrd", col);
    }

    // ---- grid: header and size validation ----------------------------------------------------
    const Bytes good = read_file("tests/data/grid-noise.ggrd");
    CHECK(good.size() > kGridCodecHeaderBytes);
    {
        Bytes cells;
        GridCodecInfo info;
        CHECK(grid_codec_info(nullptr, 0, info) == GridCodecError::TooShort);
        for (std::size_t n = 0; n < kGridCodecHeaderBytes; ++n) {
            CHECK(grid_codec_info(good.data(), n, info) == GridCodecError::TooShort);
        }
        for (std::size_t offset : {0U, 1U, 2U, 3U}) { // magic
            Bytes bad = good;
            bad[offset] ^= 0x01;
            CHECK(decode(bad, cells) == GridCodecError::BadHeader);
        }
        Bytes bad = good;
        store_le16(bad.data() + 4, 2); // version
        CHECK(decode(bad, cells) == GridCodecError::BadHeader);
        bad = good;
        store_le16(bad.data() + 6, 1); // flags
        CHECK(decode(bad, cells) == GridCodecError::BadHeader);
        bad = good;
        store_le32(bad.data() + 8, 0); // zero width, with a consistent cell count
        store_le32(bad.data() + 16, 0);
        CHECK(decode(bad, cells) == GridCodecError::BadHeader);
        bad = good;
        store_le32(bad.data() + 12, 0); // zero height, with a consistent cell count
        store_le32(bad.data() + 16, 0);
        CHECK(decode(bad, cells) == GridCodecError::BadHeader);
        bad = good;
        store_le32(bad.data() + 16, load_le32(good.data() + 16) + 1); // cells != w*h
        CHECK(decode(bad, cells) == GridCodecError::BadHeader);
        bad = good;
        store_le32(bad.data() + 8, 65536);
        store_le32(bad.data() + 12, 65536); // 2^32 cells: must not wrap to a small number
        store_le32(bad.data() + 16, 0);
        CHECK(decode(bad, cells) == GridCodecError::TooLarge);
        bad = good;
        store_le32(bad.data() + 8, 0xFFFFFFFFU);
        store_le32(bad.data() + 12, 0xFFFFFFFFU);
        store_le32(bad.data() + 16, 1);
        CHECK(decode(bad, cells) == GridCodecError::TooLarge);
        bad = good;
        store_le32(bad.data() + 8, kGridCodecMaxCells);
        store_le32(bad.data() + 12, 2);
        store_le32(bad.data() + 16, kGridCodecMaxCells * 2U);
        CHECK(decode(bad, cells) == GridCodecError::TooLarge);
        bad = good; // exactly the cell limit is fine for the header, one more is not
        store_le32(bad.data() + 8, kGridCodecMaxCells);
        store_le32(bad.data() + 12, 1);
        store_le32(bad.data() + 16, kGridCodecMaxCells);
        CHECK(grid_codec_info(bad.data(), bad.size(), info) == GridCodecError::Ok);
        store_le32(bad.data() + 8, kGridCodecMaxCells + 1U);
        store_le32(bad.data() + 16, kGridCodecMaxCells + 1U);
        CHECK(grid_codec_info(bad.data(), bad.size(), info) == GridCodecError::TooLarge);
        bad = good;
        store_le32(bad.data() + 20, load_le32(good.data() + 20) + 1); // payload size field
        CHECK(decode(bad, cells) == GridCodecError::SizeMismatch);
        bad = good;
        bad.push_back(0); // data longer than the field says
        CHECK(decode(bad, cells) == GridCodecError::SizeMismatch);
        bad = good;
        bad.pop_back();
        CHECK(decode(bad, cells) == GridCodecError::SizeMismatch);
        bad = good;
        bad[kGridCodecHeaderBytes + 3] ^= 0x10; // payload bit flip: caught by the payload CRC
        CHECK(decode(bad, cells) == GridCodecError::BadChecksum);
        bad = good;
        store_le32(bad.data() + 28, load_le32(good.data() + 28) ^ 1U); // cells CRC field
        CHECK(decode(bad, cells) == GridCodecError::BadChecksum);

        // Output buffer limits.
        std::vector<std::uint8_t> small(10);
        CHECK(grid_codec_decode(good.data(), good.size(), small.data(), small.size()) ==
              GridCodecError::OutputTooSmall);
        CHECK(grid_codec_decode(good.data(), good.size(), nullptr, 0) ==
              GridCodecError::OutputTooSmall);
        // An exactly sized buffer is enough and nothing is written past it.
        std::vector<std::uint8_t> exact(37 * 23 + 8, 0xEE);
        CHECK(grid_codec_decode(good.data(), good.size(), exact.data(), 37 * 23) ==
              GridCodecError::Ok);
        for (std::size_t i = 37 * 23; i < exact.size(); ++i) {
            CHECK(exact[i] == 0xEE);
        }
    }

    // ---- grid: stream-level validation (container checks satisfied, stream hostile) -----------
    {
        Bytes cells;
        Bytes bad = good;
        bad.push_back(0); // one unused trailing payload byte
        refresh(bad);
        CHECK(decode(bad, cells) == GridCodecError::BadStream);
        bad = good;
        bad.pop_back(); // the decoder runs out of input
        refresh(bad);
        CHECK(decode(bad, cells) != GridCodecError::Ok);
        bad = good;
        for (int i = 0; i < 4; ++i) { // first code word >= range: no valid stream starts so
            bad[kGridCodecHeaderBytes + static_cast<std::size_t>(i)] = 0xFF;
        }
        refresh(bad);
        CHECK(decode(bad, cells) == GridCodecError::BadStream);
        bad = good;
        bad.resize(kGridCodecHeaderBytes + 3); // payload shorter than the 4-byte start
        refresh(bad);
        CHECK(decode(bad, cells) == GridCodecError::BadStream);
        bad.resize(kGridCodecHeaderBytes); // empty payload
        refresh(bad);
        CHECK(decode(bad, cells) == GridCodecError::BadStream);
    }

    // ---- grid: every truncation and every single-bit flip is rejected, never crashes ---------
    {
        Bytes cells;
        for (std::size_t n = 0; n < good.size(); ++n) {
            const Bytes cut(good.begin(), good.begin() + static_cast<std::ptrdiff_t>(n));
            CHECK(decode(cut, cells) != GridCodecError::Ok);
        }
        for (std::size_t byte = 0; byte < good.size(); ++byte) {
            for (unsigned bit = 0; bit < 8; ++bit) {
                Bytes bad = good;
                bad[byte] ^= static_cast<std::uint8_t>(1U << bit);
                CHECK(decode(bad, cells) != GridCodecError::Ok);
            }
        }
        // Payload changes that survive the container checks (CRC refreshed): decoding must not
        // crash. The last few bytes of a range-coder stream are flush slack that can change without
        // changing the output, so Ok is allowed only when the cells are still exactly the original.
        const Bytes original = read_file("tests/data/grid-noise.cells");
        for (std::size_t byte = kGridCodecHeaderBytes; byte < good.size(); ++byte) {
            for (std::uint8_t mask : {0x01, 0x80, 0xFF}) {
                Bytes bad = good;
                bad[byte] ^= mask;
                refresh(bad);
                const GridCodecError e = decode(bad, cells);
                CHECK(e != GridCodecError::Ok || cells == original);
            }
        }
    }

    // ---- points: the real atlas ---------------------------------------------------------------
    {
        const Bytes gatl = read_file("data/atlas.gatl");
        const Bytes stream = read_file("tests/data/atlas-points.dvar");
        AtlasError error = AtlasError::Ok;
        const auto atlas = Atlas::parse(gatl.data(), gatl.size(), &error);
        CHECK(atlas.has_value());
        if (atlas) {
            AtlasCursor cursor;
            AtlasPolyline line;
            std::size_t pos = 0;
            std::size_t points = 0;
            bool all_equal = true;
            while (atlas->next(cursor, line)) {
                std::vector<std::uint16_t> out(2U * line.point_count);
                std::size_t used = 0;
                const PointCodecError e = decode_delta_points(
                    stream.data() + pos, stream.size() - pos, &used, out.data(), line.point_count);
                CHECK(e == PointCodecError::Ok);
                if (e != PointCodecError::Ok) {
                    break;
                }
                for (std::size_t i = 0; i < line.point_count; ++i) {
                    all_equal = all_equal && out[2 * i] == load_le16(line.points + 4 * i) &&
                                out[2 * i + 1] == load_le16(line.points + 4 * i + 2);
                }
                pos += used;
                points += line.point_count;
            }
            CHECK(all_equal);
            CHECK(pos == stream.size()); // every byte was used
            CHECK(points == atlas->point_count());
            CHECK(stream.size() < 4U * points); // smaller than the 4 bytes per point of the atlas
        }
    }

    // ---- points: hand-made vectors and hostile input ------------------------------------------
    {
        std::uint16_t out[8] = {};
        std::size_t used = 99;
        // (0,0) then (65535,65535) then back to (0,0): the largest deltas, 3 bytes each.
        const Bytes extreme = {0x00, 0x00, 0xFE, 0xFF, 0x07, 0xFE, 0xFF,
                               0x07, 0xFD, 0xFF, 0x07, 0xFD, 0xFF, 0x07};
        CHECK(decode_delta_points(extreme.data(), extreme.size(), &used, out, 3) ==
              PointCodecError::Ok);
        CHECK(used == extreme.size());
        CHECK(out[0] == 0 && out[1] == 0 && out[2] == 65535 && out[3] == 65535 && out[4] == 0 &&
              out[5] == 0);
        // First point is absolute: x=300 (zigzag 600 = 0xD8 0x04), y=5 (zigzag 10 = 0x0A).
        const Bytes first = {0xD8, 0x04, 0x0A, 0x01, 0x02};
        CHECK(decode_delta_points(first.data(), first.size(), &used, out, 2) ==
              PointCodecError::Ok);
        CHECK(used == 5 && out[0] == 300 && out[1] == 5 && out[2] == 299 && out[3] == 6);
        // Zero points consume nothing.
        used = 99;
        CHECK(decode_delta_points(first.data(), 0, &used, out, 0) == PointCodecError::Ok);
        CHECK(used == 0);
        // Truncated: ends inside the second varint, or between x and y.
        CHECK(decode_delta_points(first.data(), 2, &used, out, 1) == PointCodecError::Truncated);
        CHECK(decode_delta_points(first.data(), 3, &used, out, 2) == PointCodecError::Truncated);
        CHECK(decode_delta_points(first.data(), 0, &used, out, 1) == PointCodecError::Truncated);
        // A varint of four bytes, or a non-shortest one, is rejected.
        const Bytes four = {0x80, 0x80, 0x80, 0x01, 0x00};
        CHECK(decode_delta_points(four.data(), four.size(), &used, out, 1) ==
              PointCodecError::BadVarint);
        const Bytes padded = {0x80, 0x00, 0x00};
        CHECK(decode_delta_points(padded.data(), padded.size(), &used, out, 1) ==
              PointCodecError::BadVarint);
        const Bytes padded3 = {0x80, 0x80, 0x00, 0x00};
        CHECK(decode_delta_points(padded3.data(), padded3.size(), &used, out, 1) ==
              PointCodecError::BadVarint);
        // Coordinates leaving 0..65535 are rejected, on either axis and either side.
        const Bytes below_x = {0x01, 0x00}; // dx = -1 from 0
        CHECK(decode_delta_points(below_x.data(), below_x.size(), &used, out, 1) ==
              PointCodecError::OutOfRange);
        const Bytes below_y = {0x00, 0x01};
        CHECK(decode_delta_points(below_y.data(), below_y.size(), &used, out, 1) ==
              PointCodecError::OutOfRange);
        const Bytes above_x = {0xFE, 0xFF, 0x07, 0x00, 0x02, 0x00}; // (65535,0) then dx = +1
        CHECK(decode_delta_points(above_x.data(), above_x.size(), &used, out, 2) ==
              PointCodecError::OutOfRange);
        const Bytes above_y = {0x00, 0xFE, 0xFF, 0x07, 0x00, 0x02};
        CHECK(decode_delta_points(above_y.data(), above_y.size(), &used, out, 2) ==
              PointCodecError::OutOfRange);
        // The first point of the extreme vector decodes alone; cutting inside the second is caught.
        CHECK(decode_delta_points(extreme.data(), 5, &used, out, 2) == PointCodecError::Truncated);
    }

    return TEST_MAIN_RESULT();
}
