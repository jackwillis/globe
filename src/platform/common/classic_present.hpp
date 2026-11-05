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
        active_ = true;
        dirty_ = false;
        return true;
    }
    bool active(std::uint64_t generation) const { return active_ && generation == generation_; }
    bool pending(std::uint64_t generation) const {
        return generation == generation_ && (active_ || dirty_);
    }
    int first() const { return next_; }
    int end() const { return std::min(height_, next_ + 32); }
    void submitted(std::uint64_t generation) {
        if (!active(generation))
            return;
        next_ = end();
        active_ = next_ < height_;
    }

  private:
    std::uint64_t generation_ = 0;
    int height_ = 0, next_ = 0;
    bool active_ = false, dirty_ = false;
};
} // namespace globe
