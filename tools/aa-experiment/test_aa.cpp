// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "aa.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>

using aa_experiment::Chain;
using globe::ScreenPoint;
unsigned checks = 0, failures = 0;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(x)) {                                                                                \
            ++failures;                                                                            \
            std::fprintf(stderr, "line %d: %s\n", __LINE__, #x);                                   \
        }                                                                                          \
    } while (false)
bool equal(const globe::Framebuffer& a, const globe::Framebuffer& b) {
    return std::equal(a.data(), a.data() + a.width() * a.height(), b.data());
}
std::uint8_t* fail_allocate(std::size_t) noexcept {
    return nullptr;
}
int main() {
    using namespace aa_experiment;
    CHECK(!Workspace::create(0, 48, 10000));
    CHECK(!Workspace::create(4097, 48, 1000000));
    CHECK(!Workspace::create(48, 48, 48 * 48 - 1));
    CHECK(!Workspace::create(48, 48, 48 * 48, fail_allocate));
    auto ws = Workspace::create(97, 73, 97 * 73);
    auto a = globe::Framebuffer::create(97, 73), b = globe::Framebuffer::create(97, 73);
    CHECK(ws && a && b);
    if (!ws || !a || !b)
        return 1;
    CHECK(ws->bytes() == 97 * 73);
    CHECK(blend(0, 0xffff, 128) == 0x8410);
    CHECK(blend(0x001f, 0xf800, 128) == 0x800f);
    CHECK(blend(0x001f, 0xf800, 127) == 0x7810);
    CHECK(blend(0, 0xffff, 1) == 0);
    for (unsigned p = 0; p < 65536; ++p) {
        CHECK(blend(p, static_cast<globe::Rgb565>(p ^ 0xffff), 0) == p);
        CHECK(blend(static_cast<globe::Rgb565>(p ^ 0xffff), p, 255) == p);
        CHECK(blend(p, p, 128) == p);
    }
    const std::array<ScreenPoint, 8> directions = {
        {{1, .5}, {.5, 1}, {-.5, 1}, {-1, .5}, {-1, -.5}, {-.5, -1}, {.5, -1}, {1, -.5}}};
    for (double length : {.25, .5, 1., 3., 16., 256.})
        for (double fx : {0., .25, .5, .75})
            for (double fy : {0., .25, .5, .75})
                for (auto v : directions) {
                    double norm = std::hypot(v.x, v.y);
                    std::array<ScreenPoint, 2> points = {
                        {{30 + fx, 30 + fy},
                         {30 + fx + v.x * length / norm, 30 + fy + v.y * length / norm}}};
                    Chain c{points};
                    a->clear(0);
                    b->clear(0);
                    CHECK(render_feature(*a, {&c, 1}, 0xffff, *ws) == Result::Ok);
                    std::reverse(points.begin(), points.end());
                    CHECK(render_feature(*b, {&c, 1}, 0xffff, *ws) == Result::Ok);
                    CHECK(equal(*a, *b));
                    CHECK(
                        std::any_of(a->data(), a->data() + 97 * 73, [](auto p) { return p != 0; }));
                }
    std::array<ScreenPoint, 2> horizontal = {{{3.5, 10.5}, {19.5, 10.5}}};
    Chain h{horizontal};
    a->clear(0);
    CHECK(render_feature(*a, {&h, 1}, 0xffff, *ws) == Result::Ok);
    CHECK(a->pixel(3, 10) == blend(0, 0xffff, 128));
    CHECK(a->pixel(19, 10) == a->pixel(3, 10));
    for (int x = 4; x < 19; ++x)
        CHECK(a->pixel(x, 10) == 0xffff);
    CHECK(a->pixel(2, 10) == 0 && a->pixel(20, 10) == 0);
    for (double angle : {15., 45., 90., 150.})
        for (bool closed : {false, true}) {
            const double t = angle * 3.14159265358979323846 / 180;
            std::array<ScreenPoint, 4> points = {
                {{8.25, 20.75},
                 {20.25, 20.75},
                 {20.25 + 12 * std::cos(t), 20.75 - 12 * std::sin(t)},
                 {12.25, 12.75}}};
            Chain c{points, closed};
            a->clear(0);
            b->clear(0);
            CHECK(render_feature(*a, {&c, 1}, 0xf81f, *ws) == Result::Ok);
            std::reverse(points.begin(), points.end());
            CHECK(render_feature(*b, {&c, 1}, 0xf81f, *ws) == Result::Ok);
            CHECK(equal(*a, *b));
        }
    std::array<ScreenPoint, 6> self = {{{8.25, 8.75},
                                        {24.25, 24.75},
                                        {8.25, 24.75},
                                        {24.25, 8.75},
                                        {8.25, 24.75},
                                        {24.25, 24.75}}};
    std::array<Chain, 2> twice = {Chain{self}, Chain{self}};
    a->clear(0x4208);
    b->clear(0x4208);
    CHECK(render_feature(*a, {twice.data(), 1}, 0xffff, *ws) == Result::Ok);
    CHECK(render_feature(*b, twice, 0xffff, *ws) == Result::Ok);
    CHECK(equal(*a, *b));
    std::array<ScreenPoint, 5> dup = {
        {{3.5, 10.5}, {3.5, 10.5}, {19.5, 10.5}, {19.5, 10.5}, {19.5, 10.5}}};
    Chain d{dup};
    a->clear(0);
    b->clear(0);
    CHECK(render_feature(*a, {&h, 1}, 0xffff, *ws) == Result::Ok);
    CHECK(render_feature(*b, {&d, 1}, 0xffff, *ws) == Result::Ok);
    CHECK(equal(*a, *b));
    std::array<ScreenPoint, 2> zero = {{{20.25, 20.75}, {20.25, 20.75}}};
    Chain z{zero};
    b->clear(0);
    CHECK(render_feature(*b, {&z, 1}, 0xffff, *ws) == Result::Ok);
    CHECK(std::all_of(b->data(), b->data() + 97 * 73, [](auto p) { return p == 0; }));
    for (double bad : {std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity(), 1e100, 1000001.}) {
        std::array<ScreenPoint, 2> invalid = {{{3, 3}, {bad, 4}}};
        Chain c{invalid};
        b->clear(0x1234);
        CHECK(render_feature(*b, {&c, 1}, 0xffff, *ws) == Result::InvalidInput);
        CHECK(std::all_of(b->data(), b->data() + 97 * 73, [](auto p) { return p == 0x1234; }));
    }
    std::array<ScreenPoint, 2> cross = {{{-1000000, -1000000}, {1000000, 1000000}}};
    Chain c{cross};
    a->clear(0);
    b->clear(0);
    CHECK(render_feature(*a, {&c, 1}, 0xffff, *ws) == Result::Ok);
    b->clip_rows(9, 17);
    CHECK(render_feature(*b, {&c, 1}, 0xffff, *ws) == Result::Ok);
    b->clip_rows(0, 73);
    for (int y = 0; y < 73; ++y)
        for (int x = 0; x < 97; ++x)
            CHECK(b->pixel(x, y) == (y >= 9 && y < 17 ? a->pixel(x, y) : 0));
    std::array<ScreenPoint, 2> outside = {{{-.25, 8.5}, {-.25, 20.5}}};
    Chain o{outside};
    b->clear(0);
    CHECK(render_feature(*b, {&o, 1}, 0xffff, *ws) == Result::Ok);
    CHECK(b->pixel(0, 12) != 0);
    auto wrong = Workspace::create(48, 48, 10000);
    CHECK(wrong);
    CHECK(render_feature(*b, {&o, 1}, 0xffff, *wrong) == Result::WorkspaceMismatch);
    std::printf("%u checks, %u failures; scratch=%zu bytes; workspace object=%zu bytes\n", checks,
                failures, ws->bytes(), sizeof(Workspace));
    return failures ? 1 : 0;
}
