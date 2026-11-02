// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/framebuffer.hpp"
#include "globe/raster.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

using namespace globe;

namespace {

constexpr Rgb565 kBlank = 0xFFFF;

// Palette whose color equals its index plus an offset, so a pixel value reveals the index used.
std::array<Rgb565, 256> ramp(int offset = 0) {
    std::array<Rgb565, 256> p{};
    for (int i = 0; i < 256; ++i) {
        p[static_cast<std::size_t>(i)] = static_cast<Rgb565>(i + offset);
    }
    return p;
}

Framebuffer make(int w, int h) {
    auto fb = Framebuffer::create(w, h);
    if (!fb) {
        std::abort();
    }
    fb->clear(kBlank);
    return std::move(*fb);
}

std::uint32_t lcg_state = 12345;
double rnd(double lo, double hi) {
    lcg_state = lcg_state * 1664525U + 1013904223U;
    return lo + (hi - lo) * ((lcg_state >> 8) / 16777216.0);
}

double edge(double ax, double ay, double bx, double by, double px, double py) {
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

int covered(const Framebuffer& fb) {
    int n = 0;
    for (int y = 0; y < fb.height(); ++y) {
        for (int x = 0; x < fb.width(); ++x) {
            n += fb.pixel(x, y) != kBlank;
        }
    }
    return n;
}

void tri(Framebuffer& fb, const double* p, const Rgb565* pal) {
    fill_triangle_shaded(fb, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], pal);
}

} // namespace

int main() {
    const auto pal = ramp();

    // Constant value: every covered pixel gets palette[floor(v)], nothing else is touched.
    {
        Framebuffer fb = make(40, 40);
        const double t[] = {5.5, 4.25, 100.9, 33.0, 12.0, 100.9, 10.0, 35.5, 100.9};
        tri(fb, t, pal.data());
        int n = 0;
        for (int y = 0; y < 40; ++y) {
            for (int x = 0; x < 40; ++x) {
                const Rgb565 p = fb.pixel(x, y);
                CHECK(p == kBlank || p == 100);
                n += p == 100;
            }
        }
        // Area is about 0.5*|det|; coverage must be close to it.
        const double area = 0.5 * std::fabs(edge(t[0], t[1], t[3], t[4], t[6], t[7]));
        CHECK(std::fabs(n - area) < 0.1 * area);
    }

    // Coverage agrees with the half-plane test for pixel centers not close to an edge.
    for (int trial = 0; trial < 200; ++trial) {
        Framebuffer fb = make(48, 48);
        double t[9];
        for (int i = 0; i < 3; ++i) {
            t[i * 3] = rnd(-10, 58);
            t[i * 3 + 1] = rnd(-10, 58);
            t[i * 3 + 2] = rnd(0, 255);
        }
        const double det = edge(t[0], t[1], t[3], t[4], t[6], t[7]);
        if (std::fabs(det) < 1.0) {
            continue;
        }
        tri(fb, t, pal.data());
        for (int y = 0; y < 48; ++y) {
            for (int x = 0; x < 48; ++x) {
                const double px = x + 0.5;
                const double py = y + 0.5;
                const double e0 = edge(t[0], t[1], t[3], t[4], px, py) / det;
                const double e1 = edge(t[3], t[4], t[6], t[7], px, py) / det;
                const double e2 = edge(t[6], t[7], t[0], t[1], px, py) / det;
                const double m = std::min({e0, e1, e2});
                // Distance scale: barycentric 1e-6 of a >=1-area triangle is far below a pixel.
                if (m > 1.0e-6) {
                    CHECK(fb.pixel(x, y) != kBlank);
                } else if (m < -1.0e-6) {
                    CHECK(fb.pixel(x, y) == kBlank);
                }
            }
        }
    }

    // Two triangles sharing an edge cover every pixel exactly once, whatever the vertex order
    // or winding they are given in.
    for (int trial = 0; trial < 200; ++trial) {
        double q[8];
        // Convex quad: jittered corners of a square.
        const double jx[4] = {0, 1, 1, 0};
        const double jy[4] = {0, 0, 1, 1};
        for (int i = 0; i < 4; ++i) {
            q[i * 2] = 8 + jx[i] * 30 + rnd(-6, 6);
            q[i * 2 + 1] = 8 + jy[i] * 30 + rnd(-6, 6);
        }
        const auto pa = ramp(0);
        const auto pb = ramp(1000);
        Framebuffer fa = make(48, 48);
        Framebuffer fb = make(48, 48);
        const int order = trial % 6;
        const int perm[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
        const int a[3] = {0, 1, 2}; // triangle 0-1-2
        const int b[3] = {0, 2, 3}; // triangle 0-2-3, sharing the 0-2 diagonal
        double ta[9];
        double tb[9];
        for (int i = 0; i < 3; ++i) {
            const int ia = a[perm[order][i]];
            const int ib = b[perm[(order + 3) % 6][i]];
            ta[i * 3] = q[ia * 2];
            ta[i * 3 + 1] = q[ia * 2 + 1];
            ta[i * 3 + 2] = 10;
            tb[i * 3] = q[ib * 2];
            tb[i * 3 + 1] = q[ib * 2 + 1];
            tb[i * 3 + 2] = 20;
        }
        tri(fa, ta, pa.data());
        tri(fb, tb, pb.data());
        for (int y = 0; y < 48; ++y) {
            for (int x = 0; x < 48; ++x) {
                const bool in_a = fa.pixel(x, y) != kBlank;
                const bool in_b = fb.pixel(x, y) != kBlank;
                CHECK(!(in_a && in_b));
                const double px = x + 0.5;
                const double py = y + 0.5;
                // Strictly inside the quad (all four outer edges), away from them.
                double m = 1e9;
                for (int i = 0; i < 4; ++i) {
                    const int j = (i + 1) % 4;
                    m = std::min(m, edge(q[i * 2], q[i * 2 + 1], q[j * 2], q[j * 2 + 1], px, py));
                }
                if (m > 1.0e-6) {
                    CHECK(in_a != in_b);
                }
                if (m < -1.0e-6) {
                    CHECK(!in_a && !in_b);
                }
            }
        }
    }

    // Vertex order and winding do not change the picture.
    {
        const double v[3][3] = {{3.3, 2.1, 20}, {37.7, 9.4, 200}, {15.2, 36.8, 90}};
        Framebuffer ref = make(40, 40);
        const double t0[] = {v[0][0], v[0][1], v[0][2], v[1][0], v[1][1],
                             v[1][2], v[2][0], v[2][1], v[2][2]};
        tri(ref, t0, pal.data());
        CHECK(covered(ref) > 100);
        const int perm[5][3] = {{0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
        for (const auto& p : perm) {
            Framebuffer fb = make(40, 40);
            double t[9];
            for (int i = 0; i < 3; ++i) {
                for (int k = 0; k < 3; ++k) {
                    t[i * 3 + k] = v[p[i]][k];
                }
            }
            tri(fb, t, pal.data());
            for (int y = 0; y < 40; ++y) {
                for (int x = 0; x < 40; ++x) {
                    CHECK(fb.pixel(x, y) == ref.pixel(x, y));
                }
            }
        }
    }

    // The interpolant is the plane through the vertex values: check against the exact plane.
    for (int trial = 0; trial < 100; ++trial) {
        Framebuffer fb = make(64, 64);
        const double ax = rnd(-0.02, 0.02) * 100;
        const double ay = rnd(-0.02, 0.02) * 100;
        const double c = rnd(60, 190);
        double t[9];
        for (int i = 0; i < 3; ++i) {
            t[i * 3] = rnd(5, 59);
            t[i * 3 + 1] = rnd(5, 59);
            t[i * 3 + 2] = c + ax * (t[i * 3] - 32) + ay * (t[i * 3 + 1] - 32);
        }
        if (std::fabs(edge(t[0], t[1], t[3], t[4], t[6], t[7])) < 20) {
            continue;
        }
        tri(fb, t, pal.data());
        int seen = 0;
        for (int y = 0; y < 64; ++y) {
            for (int x = 0; x < 64; ++x) {
                const Rgb565 p = fb.pixel(x, y);
                if (p == kBlank) {
                    continue;
                }
                ++seen;
                const double want = c + ax * (x + 0.5 - 32) + ay * (y + 0.5 - 32);
                CHECK(std::fabs(p - std::floor(want)) <= 1.0);
            }
        }
        CHECK(seen > 0);
    }

    // Out-of-range values clamp to the first and last palette entries.
    {
        Framebuffer fb = make(30, 30);
        const double lo[] = {2, 2, -50, 28, 3, -1e300, 10, 28, -7};
        tri(fb, lo, pal.data());
        CHECK(covered(fb) > 50);
        for (int y = 0; y < 30; ++y) {
            for (int x = 0; x < 30; ++x) {
                CHECK(fb.pixel(x, y) == kBlank || fb.pixel(x, y) == 0);
            }
        }
        Framebuffer fh = make(30, 30);
        const double hi[] = {2, 2, 999, 28, 3, 256, 10, 28, 1e300};
        tri(fh, hi, pal.data());
        for (int y = 0; y < 30; ++y) {
            for (int x = 0; x < 30; ++x) {
                CHECK(fh.pixel(x, y) == kBlank || fh.pixel(x, y) == 255);
            }
        }
    }

    // Clamping happens at the vertices, before interpolation: the plane runs through the clamped
    // values, not through the original ones.
    {
        Framebuffer fb = make(60, 60);
        const double t[] = {5, 5, -300, 55, 5, 100, 5, 55, 100};
        tri(fb, t, pal.data());
        // Clamped vertex values 0, 100, 100 give the plane v = 2 * ((x - 5) + (y - 5)).
        int seen = 0;
        for (int y = 0; y < 60; ++y) {
            for (int x = 0; x < 60; ++x) {
                const Rgb565 p = fb.pixel(x, y);
                if (p == kBlank) {
                    continue;
                }
                ++seen;
                const double want = 2.0 * ((x + 0.5 - 5) + (y + 0.5 - 5));
                CHECK(std::fabs(p - std::floor(want)) <= 1.0);
            }
        }
        CHECK(seen > 500);
    }

    // Clipping to the buffer: a partly off-screen triangle matches the same triangle drawn into
    // a larger buffer, restricted to the window. Coordinates are multiples of 1/16, so the
    // shifted copy is exact.
    for (int trial = 0; trial < 100; ++trial) {
        double t[9];
        for (int i = 0; i < 3; ++i) {
            t[i * 3] = std::floor(rnd(-40, 70) * 16) / 16;
            t[i * 3 + 1] = std::floor(rnd(-40, 70) * 16) / 16;
            t[i * 3 + 2] = std::floor(rnd(0, 255) * 16) / 16;
        }
        Framebuffer small = make(30, 30);
        tri(small, t, pal.data());
        Framebuffer big = make(160, 160);
        double s[9];
        for (int i = 0; i < 9; ++i) {
            s[i] = t[i] + ((i % 3 == 2) ? 0 : 64);
        }
        tri(big, s, pal.data());
        for (int y = 0; y < 30; ++y) {
            for (int x = 0; x < 30; ++x) {
                CHECK(small.pixel(x, y) == big.pixel(x + 64, y + 64));
            }
        }
    }

    // The clip rows are honoured.
    {
        Framebuffer fb = make(30, 30);
        fb.clip_rows(10, 20);
        const double t[] = {0, 0, 50, 30, 0, 50, 15, 30, 50};
        tri(fb, t, pal.data());
        for (int y = 0; y < 30; ++y) {
            for (int x = 0; x < 30; ++x) {
                if (y < 10 || y >= 20) {
                    CHECK(fb.pixel(x, y) == kBlank);
                }
            }
        }
        CHECK(fb.pixel(15, 15) == 50);
    }

    // Degenerate and invalid input draws nothing.
    {
        const double nan = std::numeric_limits<double>::quiet_NaN();
        const double inf = std::numeric_limits<double>::infinity();
        const double bad[][9] = {
            {1, 1, 5, 10, 10, 5, 20, 20, 5}, // collinear
            {1, 1, 5, 1, 1, 5, 1, 1, 5},     // a point
            {nan, 1, 5, 20, 1, 5, 1, 20, 5}, // NaN coordinate
            {1, 1, nan, 20, 1, 5, 1, 20, 5}, // NaN value
            {inf, 1, 5, 20, 1, 5, 1, 20, 5}, // infinite coordinate
            {1, 1, 5, 2e6, 1, 5, 1, 20, 5},  // beyond the supported box
            {1, 1, inf, 20, 1, 5, 1, 20, 5}, // infinite value
        };
        for (const auto& t : bad) {
            Framebuffer fb = make(30, 30);
            tri(fb, t, pal.data());
            CHECK(covered(fb) == 0);
        }
    }

    return TEST_MAIN_RESULT();
}
