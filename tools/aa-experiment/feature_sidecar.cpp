// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Host-only isolated feature rendering; the included candidate and extractor are unchanged.
#define main unused_fixture_export_main
#include "fixtures.cpp"
#undef main
#include <cstdlib>

struct Reader {
    const Bytes& b;
    std::size_t pos = 8;
    bool ok = true;
    std::uint64_t integer(unsigned n) {
        if (!ok || pos + n > b.n) {
            ok = false;
            return 0;
        }
        std::uint64_t v = 0;
        for (unsigned i = 0; i < n; ++i)
            v |= std::uint64_t(b.p[pos++]) << (8 * i);
        return v;
    }
    double number() { return std::bit_cast<double>(integer(8)); }
};
int main(int argc, char** argv) {
    if (argc != 7)
        return 2; // geometry background width height feature-index A|B
    int width = std::atoi(argv[3]), height = std::atoi(argv[4]);
    unsigned wanted = static_cast<unsigned>(std::strtoul(argv[5], nullptr, 10));
    if (width < 1 || height < 1 || width > 640 || height > 480)
        return 2;
    const std::string arm = argv[6];
    if (arm != "A" && arm != "B")
        return 2;
    Bytes raw, bg;
    if (!raw.load(argv[1]) || !bg.load(argv[2]) || raw.n < 12 ||
        std::memcmp(raw.p.get(), "PEAAAB1\0", 8) != 0 || bg.n != std::size_t(width) * height * 2)
        return 3;
    Fixture f;
    if (!f.ok())
        return 4;
    Reader r{raw};
    unsigned nf = static_cast<unsigned>(r.integer(4));
    if (nf > MaxFeatures || wanted >= nf)
        return 3;
    for (unsigned i = 0; i < nf; ++i) {
        unsigned id = static_cast<unsigned>(r.integer(4));
        auto color = static_cast<Rgb565>(r.integer(2));
        unsigned nc = static_cast<unsigned>(r.integer(4));
        if (nc > MaxChains - f.nc || !f.feature(color, id))
            return 3;
        auto& ft = f.features[f.nf - 1];
        for (unsigned j = 0; j < nc; ++j) {
            unsigned closed = static_cast<unsigned>(r.integer(1)),
                     np = static_cast<unsigned>(r.integer(4));
            if (closed > 1 || np < 2 || np > MaxPoints - f.np)
                return 3;
            auto& c = f.chains[f.nc++];
            c = {f.np, np, closed != 0};
            ++ft.count;
            for (unsigned k = 0; k < np; ++k) {
                ScreenPoint p{r.number(), r.number()};
                if (!r.ok || !std::isfinite(p.x) || !std::isfinite(p.y) || std::fabs(p.x) > 1e6 ||
                    std::fabs(p.y) > 1e6)
                    return 3;
                f.points[f.np++] = p;
            }
        }
        f.end_feature(false);
    }
    if (!r.ok || r.pos != raw.n)
        return 3;
    auto fb = Framebuffer::create(width, height);
    auto ws = aa_experiment::Workspace::create(width, height, 640 * 480);
    if (!fb || !ws)
        return 4;
    for (int i = 0; i < width * height; ++i)
        fb->data()[i] = static_cast<Rgb565>(bg.p[2 * i] | (unsigned(bg.p[2 * i + 1]) << 8));
    auto ft = f.features[wanted];
    if (arm == "B") {
        if (aa_experiment::render_feature(*fb, {f.borrowed.get() + ft.start, ft.count}, ft.color,
                                          *ws) != aa_experiment::Result::Ok)
            return 5;
    } else
        for (unsigned j = ft.start; j < ft.start + ft.count; ++j) {
            auto c = f.chains[j];
            for (unsigned k = 1; k < c.count; ++k) {
                auto a = f.points[c.start + k - 1], b = f.points[c.start + k];
                draw_line(*fb, a.x, a.y, b.x, b.y, ft.color);
            }
        }
    for (int i = 0; i < width * height; ++i)
        u16(stdout, fb->data()[i]);
    return std::ferror(stdout) ? 6 : 0;
}
