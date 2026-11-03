// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/grid_codec.hpp"
#include "globe/bytes.hpp"
#include "globe/crc32.hpp"

#include <array>

namespace globe {

namespace {

constexpr std::uint32_t kTopValue = 1U << 24;
constexpr unsigned kProbBits = 11;
constexpr std::uint16_t kProbInit = 1U << (kProbBits - 1);
constexpr unsigned kMoveBits = 5;
constexpr int kContexts = 6;

// Context from the local gradient |left - upper|, mirrored by the Python encoder.
constexpr std::array<std::uint8_t, 256> make_buckets() {
    std::array<std::uint8_t, 256> t{};
    for (int d = 0; d < 256; ++d) {
        t[static_cast<std::size_t>(d)] = static_cast<std::uint8_t>(d == 0   ? 0
                                                                   : d == 1 ? 1
                                                                   : d < 4  ? 2
                                                                   : d < 8  ? 3
                                                                   : d < 24 ? 4
                                                                            : 5);
    }
    return t;
}
constexpr std::array<std::uint8_t, 256> kBucket = make_buckets();

// Reads never go past `end`; a read past it sets `overrun_` and returns 0.
class RangeDecoder {
  public:
    RangeDecoder(const std::uint8_t* begin, const std::uint8_t* end) : p_(begin), end_(end) {}

    // False if the first four bytes cannot start a valid stream.
    bool start() {
        if (end_ - p_ < 4) {
            return false;
        }
        code_ = load_be32(p_);
        p_ += 4;
        return code_ < range_;
    }

    unsigned bit(std::uint16_t& prob) {
        const std::uint32_t bound = (range_ >> kProbBits) * prob;
        unsigned result;
        if (code_ < bound) {
            range_ = bound;
            prob = static_cast<std::uint16_t>(prob + ((2048U - prob) >> kMoveBits));
            result = 0;
        } else {
            code_ -= bound;
            range_ -= bound;
            prob = static_cast<std::uint16_t>(prob - (prob >> kMoveBits));
            result = 1;
        }
        while (range_ < kTopValue) {
            range_ <<= 8;
            code_ = (code_ << 8) | next();
        }
        return result;
    }

    bool ok_and_finished() const { return !overrun_ && p_ == end_; }

  private:
    std::uint32_t next() {
        if (p_ == end_) {
            overrun_ = true;
            return 0;
        }
        return *p_++;
    }

    const std::uint8_t* p_;
    const std::uint8_t* end_;
    std::uint32_t range_ = 0xFFFFFFFFU;
    std::uint32_t code_ = 0;
    bool overrun_ = false;
};

} // namespace

GridCodecError grid_codec_info(const std::uint8_t* data, std::size_t size, GridCodecInfo& info) {
    if (data == nullptr || size < kGridCodecHeaderBytes) {
        return GridCodecError::TooShort;
    }
    if (data[0] != 'G' || data[1] != 'G' || data[2] != 'R' || data[3] != 'D' ||
        load_le16(data + 4) != 1 || load_le16(data + 6) != 0) {
        return GridCodecError::BadHeader;
    }
    const std::uint32_t width = load_le32(data + 8);
    const std::uint32_t height = load_le32(data + 12);
    const std::uint32_t cells = load_le32(data + 16);
    if (width == 0 || height == 0) {
        return GridCodecError::BadHeader;
    }
    // The 64-bit product of two 32-bit values cannot wrap.
    if (static_cast<std::uint64_t>(width) * height > kGridCodecMaxCells) {
        return GridCodecError::TooLarge;
    }
    if (static_cast<std::uint64_t>(width) * height != cells) {
        return GridCodecError::BadHeader;
    }
    if (load_le32(data + 20) != size - kGridCodecHeaderBytes) {
        return GridCodecError::SizeMismatch;
    }
    info.width = width;
    info.height = height;
    return GridCodecError::Ok;
}

GridCodecError grid_codec_decode(const std::uint8_t* data, std::size_t size, std::uint8_t* out,
                                 std::size_t out_capacity, GridCodecInfo* info_out) {
    GridCodecInfo info;
    const GridCodecError header = grid_codec_info(data, size, info);
    if (header != GridCodecError::Ok) {
        return header;
    }
    if (info_out != nullptr) {
        *info_out = info;
    }
    const std::size_t cells = static_cast<std::size_t>(info.width) * info.height;
    if (out == nullptr || out_capacity < cells) {
        return GridCodecError::OutputTooSmall;
    }
    const std::uint8_t* payload = data + kGridCodecHeaderBytes;
    const std::size_t payload_size = size - kGridCodecHeaderBytes;
    if (crc32(payload, payload_size) != load_le32(data + 24)) {
        return GridCodecError::BadChecksum;
    }

    std::array<std::array<std::uint16_t, 256>, kContexts> probs;
    for (auto& row : probs) {
        row.fill(kProbInit);
    }
    RangeDecoder rc(payload, payload + payload_size);
    if (!rc.start()) {
        return GridCodecError::BadStream;
    }

    const std::size_t width = info.width;
    for (std::size_t y = 0; y < info.height; ++y) {
        std::uint8_t* row = out + y * width;
        const std::uint8_t* above = y > 0 ? row - width : nullptr;
        for (std::size_t x = 0; x < width; ++x) {
            const int left = x > 0 ? row[x - 1] : (above != nullptr ? above[0] : 128);
            const int up = above != nullptr ? above[x] : left;
            const int gradient = left > up ? left - up : up - left;
            std::uint16_t* p = probs[kBucket[static_cast<std::size_t>(gradient)]].data();
            unsigned m = 1;
            for (int k = 0; k < 8; ++k) {
                m = (m << 1) | rc.bit(p[m]);
            }
            row[x] =
                static_cast<std::uint8_t>((m + static_cast<unsigned>((left + up) >> 1)) & 255U);
        }
    }
    if (!rc.ok_and_finished()) {
        return GridCodecError::BadStream;
    }
    if (crc32(out, cells) != load_le32(data + 28)) {
        return GridCodecError::BadChecksum;
    }
    return GridCodecError::Ok;
}

} // namespace globe
