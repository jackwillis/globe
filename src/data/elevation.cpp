// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/elevation.hpp"
#include "globe/bytes.hpp"
#include "globe/crc32.hpp"
namespace globe {
namespace {
std::int32_t signed32(const std::uint8_t* p) {
    const auto v = load_le32(p);
    return static_cast<std::int32_t>(v < 0x80000000U
                                         ? static_cast<std::int64_t>(v)
                                         : static_cast<std::int64_t>(v) - 0x100000000LL);
}
std::int16_t signed16(const std::uint8_t* p) {
    const auto v = load_le16(p);
    return static_cast<std::int16_t>(v < 32768 ? static_cast<int>(v) : static_cast<int>(v) - 65536);
}
} // namespace
std::optional<Elevation> Elevation::parse(const std::uint8_t* data, std::size_t size,
                                          ElevationError* error) {
    const auto fail = [error](ElevationError why) -> std::optional<Elevation> {
        if (error)
            *error = why;
        return std::nullopt;
    };
    if (error)
        *error = ElevationError::Ok;
    if (!data || size < kElevationCellsOffset)
        return fail(ElevationError::TooShort);
    if (data[0] != 'G' || data[1] != 'E' || data[2] != 'L' || data[3] != 'V' ||
        load_le16(data + 4) != 1 || load_le16(data + 6) != 0 || load_le16(data + 32) != 256 ||
        load_le16(data + 34) != 0)
        return fail(ElevationError::BadHeader);
    const auto w = load_le32(data + 8), h = load_le32(data + 12);
    if (w == 0 || h == 0 || w > 32768 || h > 32768 ||
        static_cast<std::uint64_t>(w) * h > kElevationMaxCells)
        return fail(ElevationError::TooLarge);
    if (size != kElevationCellsOffset + static_cast<std::size_t>(w) * h)
        return fail(ElevationError::SizeMismatch);
    const auto west = signed32(data + 16), south = signed32(data + 20), east = signed32(data + 24),
               north = signed32(data + 28);
    if (west < -180000000 || east > 180000000 || west >= east || south < -90000000 ||
        north > 90000000 || south >= north)
        return fail(ElevationError::BadHeader);
    if (crc32_extend(crc32(data, 36), data + 40, size - 40) != load_le32(data + 36))
        return fail(ElevationError::BadChecksum);
    for (std::size_t i = 1; i < 256; ++i)
        if (signed16(data + 40 + 2 * i) <= signed16(data + 40 + 2 * (i - 1)))
            return fail(ElevationError::BadTable);
    return Elevation(data, w, h);
}
double Elevation::west_deg() const {
    return signed32(data_ + 16) / 1000000.0;
}
double Elevation::south_deg() const {
    return signed32(data_ + 20) / 1000000.0;
}
double Elevation::east_deg() const {
    return signed32(data_ + 24) / 1000000.0;
}
double Elevation::north_deg() const {
    return signed32(data_ + 28) / 1000000.0;
}
std::uint8_t Elevation::index(std::uint32_t x, std::uint32_t y) const {
    return x < width_ && y < height_ ? cells()[static_cast<std::size_t>(y) * width_ + x] : 0;
}
std::int16_t Elevation::value(std::uint8_t index) const {
    return signed16(data_ + 40 + 2 * static_cast<std::size_t>(index));
}
std::int16_t Elevation::meters(std::uint32_t x, std::uint32_t y) const {
    return x < width_ && y < height_ ? value(index(x, y)) : 0;
}
} // namespace globe
