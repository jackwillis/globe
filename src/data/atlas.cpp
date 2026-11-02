// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/atlas.hpp"

#include "globe/bytes.hpp"
#include "globe/crc32.hpp"

namespace globe {

namespace {

constexpr std::size_t kHeaderSize = 24;
constexpr std::uint16_t kVersion = 1;
constexpr std::uint32_t kMaxLatitudeQ = 32768;
constexpr double kQuantumDeg = 360.0 / 65536.0;

// Strict UTF-8: no overlong forms, no surrogates, nothing above U+10FFFF. Also rejects NUL.
bool valid_text(const std::uint8_t* s, std::size_t n) {
    std::size_t i = 0;
    while (i < n) {
        const std::uint8_t c = s[i];
        if (c == 0) {
            return false;
        }
        if (c < 0x80) {
            ++i;
            continue;
        }
        std::size_t extra = 0;
        std::uint32_t min = 0;
        std::uint32_t cp = 0;
        if (c >= 0xC2 && c <= 0xDF) {
            extra = 1;
            min = 0x80;
            cp = c & 0x1FU;
        } else if (c >= 0xE0 && c <= 0xEF) {
            extra = 2;
            min = 0x800;
            cp = c & 0x0FU;
        } else if (c >= 0xF0 && c <= 0xF4) {
            extra = 3;
            min = 0x10000;
            cp = c & 0x07U;
        } else {
            return false;
        }
        if (n - i <= extra) {
            return false;
        }
        for (std::size_t k = 1; k <= extra; ++k) {
            if ((s[i + k] & 0xC0U) != 0x80U) {
                return false;
            }
            cp = (cp << 6) | (s[i + k] & 0x3FU);
        }
        if (cp < min || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
            return false;
        }
        i += extra + 1;
    }
    return true;
}

} // namespace

const char* atlas_error_name(AtlasError error) {
    switch (error) {
    case AtlasError::Ok:
        return "none";
    case AtlasError::TooShort:
        return "too short";
    case AtlasError::BadMagic:
        return "bad magic";
    case AtlasError::UnsupportedVersion:
        return "unsupported version";
    case AtlasError::BadReserved:
        return "reserved field not zero";
    case AtlasError::TooLarge:
        return "count exceeds limit";
    case AtlasError::SizeMismatch:
        return "size does not match header";
    case AtlasError::BadChecksum:
        return "checksum mismatch";
    case AtlasError::BadLayer:
        return "unknown layer";
    case AtlasError::BadPolyline:
        return "malformed polyline entry";
    case AtlasError::BadPointCount:
        return "bad point count";
    case AtlasError::BadLatitude:
        return "latitude out of range";
    case AtlasError::BadText:
        return "invalid provenance text";
    }
    return "unknown";
}

LatLon AtlasPolyline::point(std::uint16_t i) const {
    if (i >= point_count) {
        return {0.0, 0.0};
    }
    const std::uint8_t* p = points + 4 * static_cast<std::size_t>(i);
    return {load_le16(p + 2) * kQuantumDeg - 90.0, load_le16(p) * kQuantumDeg - 180.0};
}

std::optional<Atlas> Atlas::parse(const std::uint8_t* data, std::size_t size, AtlasError* error) {
    const auto fail = [error](AtlasError e) -> std::optional<Atlas> {
        if (error != nullptr) {
            *error = e;
        }
        return std::nullopt;
    };
    if (data == nullptr || size < kHeaderSize) {
        return fail(AtlasError::TooShort);
    }
    if (data[0] != 'G' || data[1] != 'A' || data[2] != 'T' || data[3] != 'L') {
        return fail(AtlasError::BadMagic);
    }
    if (load_le16(data + 4) != kVersion) {
        return fail(AtlasError::UnsupportedVersion);
    }
    if (load_le16(data + 6) != 0) {
        return fail(AtlasError::BadReserved);
    }
    const std::uint32_t polylines = load_le32(data + 8);
    const std::uint32_t points = load_le32(data + 12);
    const std::uint32_t text_size = load_le32(data + 16);
    const std::uint32_t checksum = load_le32(data + 20);
    if (polylines > kAtlasMaxPolylines || points > kAtlasMaxPoints ||
        text_size > kAtlasMaxTextBytes) {
        return fail(AtlasError::TooLarge);
    }
    // The limits above keep this well inside 64 bits (and inside 32).
    const std::uint64_t expected = kHeaderSize + 4ULL * polylines + 4ULL * points + text_size;
    if (static_cast<std::uint64_t>(size) != expected) {
        return fail(AtlasError::SizeMismatch);
    }
    if (crc32(data + kHeaderSize, size - kHeaderSize) != checksum) {
        return fail(AtlasError::BadChecksum);
    }

    const std::uint8_t* table = data + kHeaderSize;
    std::uint64_t counted = 0;
    for (std::uint32_t i = 0; i < polylines; ++i) {
        const std::uint8_t* entry = table + 4ULL * i;
        if (entry[0] != static_cast<std::uint8_t>(AtlasLayer::Coastline) &&
            entry[0] != static_cast<std::uint8_t>(AtlasLayer::Border)) {
            return fail(AtlasError::BadLayer);
        }
        if (entry[1] != 0) {
            return fail(AtlasError::BadPolyline);
        }
        const std::uint32_t count = load_le16(entry + 2);
        counted += count;
        if (count < 2 || counted > points) {
            return fail(AtlasError::BadPointCount);
        }
    }
    if (counted != points) {
        return fail(AtlasError::BadPointCount);
    }

    const std::uint8_t* point_data = table + 4ULL * polylines;
    for (std::uint32_t k = 0; k < points; ++k) {
        if (load_le16(point_data + 4ULL * k + 2) > kMaxLatitudeQ) {
            return fail(AtlasError::BadLatitude);
        }
    }

    const std::uint8_t* text = point_data + 4ULL * points;
    if (!valid_text(text, text_size)) {
        return fail(AtlasError::BadText);
    }
    if (error != nullptr) {
        *error = AtlasError::Ok;
    }
    return Atlas(data, polylines, points,
                 std::string_view(reinterpret_cast<const char*>(text), text_size));
}

bool Atlas::next(AtlasCursor& cursor, AtlasPolyline& out) const {
    if (cursor.index >= polylines_) {
        return false;
    }
    const std::uint8_t* entry = data_ + kHeaderSize + 4ULL * cursor.index;
    const std::uint8_t* point_data = data_ + kHeaderSize + 4ULL * polylines_;
    out.layer = static_cast<AtlasLayer>(entry[0]);
    out.point_count = load_le16(entry + 2);
    out.points = point_data + 4ULL * cursor.first_point;
    cursor.first_point += out.point_count;
    ++cursor.index;
    return true;
}

} // namespace globe
