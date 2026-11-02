// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Reader for the bundled GATL atlas (docs/atlas-format.md): world coastlines and borders as
// polylines. An Atlas is a validated, zero-copy view over caller-owned bytes (an embedded array,
// a file read into a buffer): parsing allocates nothing and copies nothing, and the bytes must
// outlive the Atlas. Every structural rule of the format is checked once in parse(), so the
// accessors afterwards cannot read out of bounds.
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include "globe/geometry.hpp"

namespace globe {

constexpr std::uint32_t kAtlasMaxPolylines = 65536;
constexpr std::uint32_t kAtlasMaxPointsPerPolyline = 65535;
constexpr std::uint32_t kAtlasMaxPoints = 1000000;
constexpr std::uint32_t kAtlasMaxTextBytes = 4096;

enum class AtlasLayer : std::uint8_t { Coastline = 1, Border = 2 };

enum class AtlasError : std::uint8_t {
    None,
    TooShort,           // smaller than the header, or no data
    BadMagic,           // not a GATL file
    UnsupportedVersion, // a version this reader does not know
    BadReserved,        // reserved header field is not zero
    TooLarge,           // a count exceeds the documented limits
    SizeMismatch,       // byte size is not exactly header + table + points + text
    BadChecksum,        // CRC-32 of the body does not match
    BadLayer,           // unknown layer id
    BadPolyline,        // reserved table byte is not zero
    BadPointCount,      // polyline with fewer than two points, or counts disagree with the header
    BadLatitude,        // latitude outside [-90, 90]
    BadText,            // provenance text has a NUL byte or is not valid UTF-8
};

const char* atlas_error_name(AtlasError error);

// One polyline: a view of `point_count` encoded points inside the atlas bytes.
struct AtlasPolyline {
    AtlasLayer layer;
    std::uint16_t point_count;
    const std::uint8_t* points; // 4 bytes per point; use point()

    // Decoded vertex i, exact in double (the quantum is a power of two). Returns (0, 0) for
    // i >= point_count.
    LatLon point(std::uint16_t i) const;
};

// Position for walking the polylines in order; start with a default-constructed cursor.
struct AtlasCursor {
    std::uint32_t index = 0;
    std::uint32_t first_point = 0;
};

class Atlas {
  public:
    // Validates the whole file. On failure returns nullopt and, if `error` is non-null, stores
    // the first failing check. Never reads outside [data, data + size).
    static std::optional<Atlas> parse(const std::uint8_t* data, std::size_t size,
                                      AtlasError* error = nullptr);

    std::uint32_t polyline_count() const { return polylines_; }
    std::uint32_t point_count() const { return points_; }
    // Source names, versions, URLs, hashes and license, as written by the builder.
    std::string_view provenance() const { return text_; }

    // Stores the polyline at the cursor in `out` and advances the cursor. Returns false when
    // every polyline has been visited.
    bool next(AtlasCursor& cursor, AtlasPolyline& out) const;

  private:
    Atlas(const std::uint8_t* data, std::uint32_t polylines, std::uint32_t points,
          std::string_view text)
        : data_(data), polylines_(polylines), points_(points), text_(text) {}

    const std::uint8_t* data_;
    std::uint32_t polylines_;
    std::uint32_t points_;
    std::string_view text_;
};

} // namespace globe
