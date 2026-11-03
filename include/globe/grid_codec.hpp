// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Decoder for compressed one-byte-per-cell grids (GL-29 prototype, not used by any front end yet).
//
// Scheme: each cell is predicted as the average of its left and upper neighbours (already
// decoded), the residual is coded MSB first with an adaptive binary range coder (LZMA style: 11-bit
// probabilities, shift 5), with six contexts chosen from |left - upper|. The grid is one stream,
// not tiles. The matching encoder is tools/grid-codec/grid_codec.py.
//
// Container, little-endian, 32-byte header then the range-coder payload:
//   0  "GGRD"   4  version=1 (u16)   6  flags=0 (u16)   8  width (u32)   12 height (u32)
//   16 cells = width*height (u32)    20 payload bytes (u32)
//   24 CRC-32 of the payload (u32)   28 CRC-32 of the decoded cells (u32)
// Both checksums and every size are verified; the output size comes from the header and is bounded
// by kGridCodecMaxCells. The decoder never allocates and never reads or writes out of bounds.
#pragma once

#include <cstddef>
#include <cstdint>

namespace globe {

constexpr std::size_t kGridCodecHeaderBytes = 32;
constexpr std::uint32_t kGridCodecMaxCells = 16U * 1024U * 1024U;

enum class GridCodecError : std::uint8_t {
    Ok,
    TooShort,      // fewer bytes than the header
    BadHeader,     // magic, version, flags, zero dimension, or cells != width*height
    TooLarge,      // more than kGridCodecMaxCells cells
    SizeMismatch,  // payload size field disagrees with the data size
    BadChecksum,   // payload or decoded-cells CRC-32 mismatch
    BadStream,     // range coder state invalid, ran past the payload, or left bytes unused
    OutputTooSmall // caller buffer smaller than width*height
};

struct GridCodecInfo {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

// Validates the header and sizes without decoding. Returns Ok and fills `info` on success.
GridCodecError grid_codec_info(const std::uint8_t* data, std::size_t size, GridCodecInfo& info);

// Decodes into `out` (row-major, width*height bytes, rows in stored order). On any error the
// contents of `out` are unspecified. `info` is filled whenever the header is valid.
GridCodecError grid_codec_decode(const std::uint8_t* data, std::size_t size, std::uint8_t* out,
                                 std::size_t out_capacity, GridCodecInfo* info = nullptr);

} // namespace globe
