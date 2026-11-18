// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include "globe/framebuffer.hpp"
#include "globe/projection.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>

namespace aa_experiment {
struct Chain {
    std::span<const globe::ScreenPoint> points;
    bool closed = false;
};
enum class Result { Ok, InvalidInput, WorkspaceMismatch };
using Allocate = std::uint8_t* (*)(std::size_t) noexcept;
// An injected allocator must return new[]-compatible storage or nullptr.
class Workspace {
  public:
    static std::optional<Workspace> create(int width, int height, std::size_t budget,
                                           Allocate allocate = nullptr);
    std::size_t bytes() const { return static_cast<std::size_t>(width_) * height_; }
    Workspace(Workspace&&) noexcept = default;
    Workspace& operator=(Workspace&&) noexcept = default;

  private:
    Workspace(int w, int h, std::unique_ptr<std::uint8_t[]> p)
        : width_(w), height_(h), mask_(std::move(p)) {}
    int width_, height_;
    std::unique_ptr<std::uint8_t[]> mask_;
    friend Result render_feature(globe::Framebuffer&, std::span<const Chain>, globe::Rgb565,
                                 Workspace&);
};
globe::Rgb565 blend(globe::Rgb565 dst, globe::Rgb565 src, unsigned coverage);
// All visible chains of a source feature share one mask and one final blend.
// Validates the whole feature before writing. No allocation or clock reads.
Result render_feature(globe::Framebuffer&, std::span<const Chain>, globe::Rgb565, Workspace&);
} // namespace aa_experiment
