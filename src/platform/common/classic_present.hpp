// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include <algorithm>
#include <cstdint>
namespace globe {
// The adapter owns the GWorld pixels. Once captured, they stay immutable until all rows
// have been submitted. Requests arriving during submission coalesce into a later capture.
class ClassicPresentation {
  public:
    void request(std::uint64_t generation) {
        if (generation != generation_) {
            cancel();
            generation_ = generation;
        }
        dirty_ = true;
    }
    void cancel() { active_ = dirty_ = false; }
    bool needs_capture(std::uint64_t generation) const {
        return generation == generation_ && dirty_ && !active_;
    }
    bool begin(std::uint64_t generation, int height) {
        if (!needs_capture(generation) || height <= 0)
            return false;
        height_ = height;
        next_ = 0;
        slice_rows_ = rows_;
        active_ = true;
        dirty_ = false;
        return true;
    }
    bool active(std::uint64_t generation) const { return active_ && generation == generation_; }
    bool pending(std::uint64_t generation) const {
        return generation == generation_ && (active_ || dirty_);
    }
    int first() const { return next_; }
    int end() const { return std::min(height_, next_ + slice_rows_); }
    // Tuning changes the next slice, never the range currently being copied/acknowledged.
    // Rows per slice start at kStartRows (the slow Classic-under-OS-X days) and follow the machine:
    // after a slice of `rows` took `elapsed_ms`, aim the next at `target_ms`, growing at most
    // twofold a time. On bare OS 9 a whole frame is a few milliseconds, so a frame then arrives in
    // one or two slices instead of wiping in from the top.
    void tune(int rows, std::uint64_t elapsed_ms, std::uint64_t target_ms) {
        if (rows <= 0 || target_ms == 0)
            return;
        long next = elapsed_ms == 0 ? static_cast<long>(rows) * 2
                                    : static_cast<long>(rows * target_ms / elapsed_ms);
        next = std::min(next, static_cast<long>(rows_) * 2);
        rows_ = static_cast<int>(
            std::clamp(next, static_cast<long>(kMinRows), static_cast<long>(kMaxRows)));
    }
    int rows_per_slice() const { return rows_; }
    void submitted(std::uint64_t generation) {
        if (!active(generation))
            return;
        next_ = end();
        slice_rows_ = rows_;
        active_ = next_ < height_;
    }

  private:
    static constexpr int kStartRows = 32, kMinRows = 8, kMaxRows = 4096;
    int rows_ = kStartRows, slice_rows_ = kStartRows;
    std::uint64_t generation_ = 0;
    int height_ = 0, next_ = 0;
    bool active_ = false, dirty_ = false;
};
} // namespace globe
