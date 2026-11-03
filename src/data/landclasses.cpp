// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/landclasses.hpp"
#include "globe/grid_codec.hpp"
#include <cmath>
#include <new>
namespace globe {
std::optional<LandClasses> LandClasses::decode(const std::uint8_t* data, std::size_t size) {
    GridCodecInfo info;
    if (grid_codec_info(data, size, info) != GridCodecError::Ok || info.width != 360 ||
        info.height != 180)
        return std::nullopt;
    std::unique_ptr<std::uint8_t[]> cells(new (std::nothrow) std::uint8_t[bytes()]);
    if (!cells || grid_codec_decode(data, size, cells.get(), bytes()) != GridCodecError::Ok)
        return std::nullopt;
    for (std::size_t i = 0; i < bytes(); ++i)
        if (cells[i] >= kLandClassCount)
            return std::nullopt;
    return LandClasses(std::move(cells));
}
LandClass LandClasses::at(double latitude, double longitude) const {
    if (!std::isfinite(latitude) || !std::isfinite(longitude) || latitude < -90 || latitude > 90 ||
        longitude < -180 || longitude > 180)
        return LandClass::Unknown;
    const unsigned row = latitude == 90 ? 179 : static_cast<unsigned>(latitude + 90);
    const unsigned col = longitude == 180 ? 0 : static_cast<unsigned>(longitude + 180);
    return static_cast<LandClass>(cells_[static_cast<std::size_t>(row) * 360 + col]);
}
} // namespace globe
