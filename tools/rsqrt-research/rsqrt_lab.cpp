// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Research harness for #145: numerical audit and micro-benchmark of 1/sqrt candidates.
// Standalone (no globe_core); never part of the product build.
//   rsqrt-lab audit [args.bin]   error audit; args.bin = (l2, radius) double pairs logged by
//   variant 5 rsqrt-lab bench [args.bin]   throughput and dependency-chain latency, interleaved
//   blocks
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <random>
#include <vector>

#include "rsqrt_ppc.hpp"
#include "rsqrt_variants.hpp"
using namespace rsqrt_lab;

#ifndef LAB_FLAGS
#define LAB_FLAGS "unknown"
#endif

struct Cand {
    const char* name;
    double (*f)(double);
};
static double identity_fn(double x) {
    return x;
}
static const Cand kCands[] = {
    {"A_exact", exact},      {"B_current", current},  {"B2_guarded92", guarded},
    {"C1_newton1", newton1}, {"C2_taylor3", taylor3}, {"C4_seeded2", seeded2},
};

static const char* jnum(double v, char* buf) {
    if (std::isnan(v))
        return "\"nan\"";
    if (std::isinf(v))
        return v > 0 ? "\"inf\"" : "\"-inf\"";
    std::snprintf(buf, 40, "%.17g", v);
    return buf;
}
static long double ref(double x) {
    return 1.0L / sqrtl((long double)x);
}
static double relerr(double (*f)(double), double x) {
    const long double r = ref(x);
    return (double)(((long double)f(x) - r) / r);
}
static double l2_of_angle_deg(double deg) {
    const double c = std::cos(deg * M_PI / 360.0);
    return c * c;
}

struct Stat {
    double lo = 1e300, hi = -1e300, maxabs = 0, at = 0;
};
static Stat sweep(double (*f)(double), double a, double b, std::uint64_t n) {
    Stat s;
    for (std::uint64_t i = 0; i <= n; ++i) {
        const double x = a + (b - a) * (double)i / (double)n;
        const double e = relerr(f, x);
        s.lo = std::min(s.lo, e);
        s.hi = std::max(s.hi, e);
        if (std::fabs(e) > s.maxabs) {
            s.maxabs = std::fabs(e);
            s.at = x;
        }
    }
    return s;
}
static std::vector<double> read_args(const char* path, std::vector<double>* radii) {
    std::vector<double> v;
    std::FILE* f = std::fopen(path, "rb");
    if (!f)
        return v;
    double pair[2];
    while (std::fread(pair, sizeof pair, 1, f) == 1) {
        v.push_back(pair[0]);
        if (radii)
            radii->push_back(pair[1]);
    }
    std::fclose(f);
    return v;
}

static void audit(const char* argpath) {
    std::printf("{\"mode\":\"audit\",\"flags\":\"%s\",\n", LAB_FLAGS);
    std::printf(" \"p1_current_at_0.5\":{\"value\":%.17g,\"exact\":%.17g,\"rel_err\":%.6g},\n",
                current(0.5), (double)ref(0.5), relerr(current, 0.5));
    // Error of the Newton map alone, one and two steps from seed 1, at 0.5.
    {
        double s0 = 1.5 - 0.5 * 0.5;
        double s1 = s0 * (1.5 - 0.5 * 0.5 * s0 * s0);
        std::printf(" \"newton_steps_at_0.5\":{\"after_1\":%.17g,\"rel_err_1\":%.6g,\"after_2\":%."
                    "17g,\"rel_err_2\":%.6g},\n",
                    s0, (double)(((long double)s0 - ref(0.5)) / ref(0.5)), s1,
                    (double)(((long double)s1 - ref(0.5)) / ref(0.5)));
    }
    // Full documented domain [0.5, 1], dense: 2^24 samples.
    std::printf(" \"dense_full_domain_0.5_1\":{\n");
    for (std::size_t i = 0; i < sizeof kCands / sizeof *kCands; ++i) {
        const Stat s = sweep(kCands[i].f, 0.5, 1.0, 1u << 22);
        std::printf("  \"%s\":{\"min\":%.4g,\"max\":%.4g,\"maxabs\":%.4g,\"at_l2\":%.17g}%s\n",
                    kCands[i].name, s.lo, s.hi, s.maxabs, s.at,
                    i + 1 < sizeof kCands / sizeof *kCands ? "," : "");
    }
    std::printf(" },\n");
    // Declared candidate domain l2 in [0.99, 1] and shipped-data domain.
    const double shipped_deg = 5.69641166736632; // longest atlas segment (docs, see report 2.1)
    const double l2_shipped = l2_of_angle_deg(shipped_deg);
    std::printf(" \"shipped_data_limit\":{\"max_arc_deg\":%.9g,\"min_l2\":%.12g},\n", shipped_deg,
                l2_shipped);
    std::printf(" \"dense_shipped_domain\":{\n");
    for (std::size_t i = 0; i < sizeof kCands / sizeof *kCands; ++i) {
        const Stat s = sweep(kCands[i].f, l2_shipped, 1.0, 1u << 22);
        std::printf("  \"%s\":{\"min\":%.4g,\"max\":%.4g,\"maxabs\":%.4g}%s\n", kCands[i].name,
                    s.lo, s.hi, s.maxabs, i + 1 < sizeof kCands / sizeof *kCands ? "," : "");
    }
    std::printf(" },\n");
    // Per arc angle: worst error over t, i.e. at l2 = cos^2(phi/2); and monotonicity in l2.
    std::printf(" \"by_arc_angle\":[\n");
    const double angles[] = {0.5, 1, 3, 5.7, 10, 16, 20, 30, 34, 45, 60, 75, 90};
    for (std::size_t k = 0; k < sizeof angles / sizeof *angles; ++k) {
        const double l2 = l2_of_angle_deg(angles[k]);
        std::printf("  {\"arc_deg\":%g,\"min_l2\":%.10g", angles[k], l2);
        for (std::size_t i = 1; i < sizeof kCands / sizeof *kCands; ++i)
            std::printf(",\"%s\":%.4g", kCands[i].name, relerr(kCands[i].f, l2));
        std::printf(",\"B_px_r220\":%.4g,\"B_px_r440\":%.4g,\"B_px_r2560\":%.4g}%s\n",
                    220 * std::fabs(relerr(current, l2)), 440 * std::fabs(relerr(current, l2)),
                    2560 * std::fabs(relerr(current, l2)),
                    k + 1 < sizeof angles / sizeof *angles ? "," : "");
    }
    std::printf(" ],\n");
    // Where do the documented claims hold? bisect l2 for |err_B| = threshold.
    std::printf(" \"thresholds_B\":[");
    const double thr[] = {1e-4 * 0 + 2e-3, 1e-5, 1e-4, 1e-9};
    for (std::size_t k = 0; k < 4; ++k) {
        // error is monotone decreasing in l2 on [0.5,1]; find l2 with |err| = thr
        double lo = 0.5, hi = 1.0;
        for (int it = 0; it < 80; ++it) {
            const double m = 0.5 * (lo + hi);
            (std::fabs(relerr(current, m)) > thr[k] ? lo : hi) = m;
        }
        std::printf("{\"rel_err\":%g,\"min_l2\":%.8g,\"max_arc_deg\":%.5g}%s", thr[k], hi,
                    2 * std::acos(std::sqrt(hi)) * 180 / M_PI, k < 3 ? "," : "");
    }
    std::printf("],\n");
    { // one-step claim in the comment: "under 2e-3 after one"
        double lo = 0.5, hi = 1.0;
        auto one = [](double x) { return 1.5 - 0.5 * x; };
        for (int it = 0; it < 80; ++it) {
            const double m = 0.5 * (lo + hi);
            const double e = std::fabs(relerr(one, m));
            (e > 2e-3 ? lo : hi) = m;
        }
        std::printf(
            " \"one_step_2e-3\":{\"min_l2\":%.8g,\"max_arc_deg\":%.5g,\"err_at_0.5\":%.5g},\n", hi,
            2 * std::acos(std::sqrt(hi)) * 180 / M_PI, relerr(one, 0.5));
    }
    // Monotonic under-estimation property (drawn point lies between chord point and sphere).
    {
        std::uint64_t bad_under = 0, bad_over = 0, n = 1u << 20;
        for (std::size_t i = 1; i < sizeof kCands / sizeof *kCands; ++i) {
            (void)i;
        }
        for (std::uint64_t i = 0; i <= n; ++i) {
            const double x = 0.5 + 0.5 * (double)i / (double)n;
            const double y = (double)ref(x);
            if (current(x) > y * (1 + 1e-15))
                ++bad_over;
            if (current(x) < 1.0 - 1e-15)
                ++bad_under;
        }
        std::printf(" \"B_between_chord_point_and_sphere\":{\"samples\":%llu,\"over_estimates\":%"
                    "llu,\"below_1\":%llu},\n",
                    (unsigned long long)(n + 1), (unsigned long long)bad_over,
                    (unsigned long long)bad_under);
    }
    // Exceptional inputs.
    std::printf(" \"exceptional\":[\n");
    const double inf = std::numeric_limits<double>::infinity(),
                 nan = std::numeric_limits<double>::quiet_NaN();
    const double xs[] = {0.0,    -0.0,
                         -1.0,   4.9e-324,
                         1e-310, std::nextafter(0.5, 0.0),
                         0.5,    std::nextafter(0.5, 1.0),
                         1.0,    std::nextafter(1.0, 2.0),
                         2.0,    1e308,
                         inf,    nan};
    for (std::size_t k = 0; k < sizeof xs / sizeof *xs; ++k) {
        char nb[48];
        std::printf("  {\"x\":%s", jnum(xs[k], nb));
        for (std::size_t i = 0; i < sizeof kCands / sizeof *kCands; ++i) {
            char b2[48];
            std::printf(",\"%s\":%s", kCands[i].name, jnum(kCands[i].f(xs[k]), b2));
        }
        std::printf("}%s\n", k + 1 < sizeof xs / sizeof *xs ? "," : "");
    }
    std::printf(" ]");
    if (argpath) {
        std::vector<double> radii;
        const std::vector<double> v = read_args(argpath, &radii);
        if (!v.empty()) {
            std::vector<double> s = v;
            std::sort(s.begin(), s.end());
            const double mn = s.front();
            std::printf(",\n "
                        "\"logged_args\":{\"count\":%zu,\"min_l2\":%.12g,\"p0.1\":%.12g,\"p50\":%."
                        "12g,\"max\":%.12g,\"below_0.5\":%zu,\n",
                        v.size(), mn, s[s.size() / 1000], s[s.size() / 2], s.back(),
                        (std::size_t)(std::lower_bound(s.begin(), s.end(), 0.5) - s.begin()));
            std::printf("  \"equiv_max_arc_deg\":%.6g,\"max_px\":{",
                        2 * std::acos(std::sqrt(mn)) * 180 / M_PI);
            for (std::size_t i = 0; i < sizeof kCands / sizeof *kCands; ++i) {
                double worst = 0, worst_rel = 0;
                for (std::size_t j = 0; j < v.size(); ++j) {
                    const double e = std::fabs(relerr(kCands[i].f, v[j]));
                    worst_rel = std::max(worst_rel, e);
                    worst = std::max(worst, e * radii[j]);
                }
                std::printf("\"%s\":{\"max_rel\":%.4g,\"max_px\":%.4g}%s", kCands[i].name,
                            worst_rel, worst, i + 1 < sizeof kCands / sizeof *kCands ? "," : "");
            }
            std::printf("}}");
        }
    }
    std::printf("\n}\n");
}

// ---- benchmark -------------------------------------------------------------------------------
static inline void keep(const void* p) {
    asm volatile("" : : "r"(p) : "memory");
}

template <double (*F)()> struct Dummy {};
template <double (*Fn)(double)>
__attribute__((noinline)) double throughput(const double* in, double* out, int n) {
    double sum = 0;
    for (int i = 0; i < n; ++i) {
        const double r = Fn(in[i]);
        out[i] = r;
        sum += r;
    }
    keep(out);
    return sum;
}
template <double (*Fn)(double)> __attribute__((noinline)) double latency(const double* in, int n) {
    // Each input is combined with the previous result times zero: a true data dependence the
    // compiler cannot remove without -ffast-math, with identical overhead for every candidate.
    volatile double zero = 0.0;
    const double z = zero;
    double prev = 1.0;
    for (int i = 0; i < n; ++i)
        prev = Fn(in[i] + prev * z);
    keep(&prev);
    return prev;
}
struct Bench {
    const char* name;
    double (*thr)(const double*, double*, int);
    double (*lat)(const double*, int);
};
#define B(nm, fn) {nm, throughput<fn>, latency<fn>}
static const Bench kBench[] = {
    B("identity", identity_fn), B("A_exact", exact),      B("B_current", current),
    B("B2_guarded92", guarded), B("C1_newton1", newton1), B("C2_taylor3", taylor3),
    B("C4_seeded2", seeded2),
};
static double now() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

static void bench(const char* argpath) {
    const int N = 2048, REPS = 400, BLOCKS = 21;
    std::vector<double> prod(N), wide(N), out(N);
    std::mt19937_64 rng(145);
    std::uniform_real_distribution<double> u(0, 1);
    std::vector<double> logged = argpath ? read_args(argpath, nullptr) : std::vector<double>();
    for (int i = 0; i < N; ++i) {
        wide[i] = 0.5 + 0.5 * u(rng);
        const double l2min = l2_of_angle_deg(5.69641166736632);
        prod[i] = logged.empty()
                      ? l2min + (1 - l2min) * u(rng)
                      : logged[(std::size_t)(u(rng) * (double)logged.size()) % logged.size()];
    }
    const std::size_t nb = sizeof kBench / sizeof *kBench;
    struct Acc {
        std::vector<double> t_thr[2], t_lat[2];
        double sink = 0;
    };
    std::vector<Acc> acc(nb);
    const std::vector<double>* dist[2] = {&prod, &wide};
    // warm-up (discarded)
    for (std::size_t b = 0; b < nb; ++b)
        for (int d = 0; d < 2; ++d) {
            acc[b].sink += kBench[b].thr(dist[d]->data(), out.data(), N);
            acc[b].sink += kBench[b].lat(dist[d]->data(), N);
        }
    for (int blk = 0; blk < BLOCKS; ++blk) {
        for (std::size_t k = 0; k < nb; ++k) {
            const std::size_t b = (blk % 2 == 0) ? k : nb - 1 - k; // A/B/B/A order
            for (int d = 0; d < 2; ++d) {
                double t0 = now();
                for (int r = 0; r < REPS; ++r)
                    acc[b].sink += kBench[b].thr(dist[d]->data(), out.data(), N);
                acc[b].t_thr[d].push_back((now() - t0) / ((double)REPS * N) * 1e9);
                t0 = now();
                for (int r = 0; r < REPS; ++r)
                    acc[b].sink += kBench[b].lat(dist[d]->data(), N);
                acc[b].t_lat[d].push_back((now() - t0) / ((double)REPS * N) * 1e9);
            }
        }
    }
    auto med = [](std::vector<double> v) {
        std::sort(v.begin(), v.end());
        return v[v.size() / 2];
    };
    std::printf("{\"mode\":\"bench\",\"flags\":\"%s\",\"inputs\":\"%s\",\"n\":%d,\"reps\":%d,"
                "\"blocks\":%d,\"ns_per_call\":{\n",
                LAB_FLAGS, logged.empty() ? "synthetic" : "logged", N, REPS, BLOCKS);
    const char* dn[2] = {"production_domain", "full_domain_0.5_1"};
    for (std::size_t b = 0; b < nb; ++b) {
        std::printf(" \"%s\":{", kBench[b].name);
        for (int d = 0; d < 2; ++d) {
            auto& t = acc[b].t_thr[d];
            auto& l = acc[b].t_lat[d];
            std::printf("\"%s\":{\"throughput\":{\"median\":%.4f,\"min\":%.4f,\"max\":%.4f},"
                        "\"latency\":{\"median\":%.4f,\"min\":%.4f,\"max\":%.4f}}%s",
                        dn[d], med(t), *std::min_element(t.begin(), t.end()),
                        *std::max_element(t.begin(), t.end()), med(l),
                        *std::min_element(l.begin(), l.end()),
                        *std::max_element(l.begin(), l.end()), d == 0 ? "," : "");
        }
        std::printf("}%s\n", b + 1 < nb ? "," : "");
    }
    double s = 0;
    for (auto& a : acc)
        s += a.sink;
    std::printf("},\"sink\":%.6g}\n", s);
}

// ---- cross-target bit comparison and frsqrte accuracy (run under qemu-user for PowerPC)
// ----------- `bits out.bin`: results of each candidate over a fixed grid of 2^18+1 points of [0.5,
// 1], written as little-endian uint64 so a big-endian target and the host produce comparable files.
static void bits(const char* path) {
    const std::uint64_t n = 1u << 18;
    std::FILE* f = std::fopen(path, "wb");
    if (!f)
        return;
    std::printf("{\"mode\":\"bits\",\"flags\":\"%s\",\"grid\":%llu,\"fnv\":{", LAB_FLAGS,
                (unsigned long long)(n + 1));
    for (std::size_t c = 0; c < sizeof kCands / sizeof *kCands; ++c) {
        std::uint64_t h = 14695981039346656037ULL;
        for (std::uint64_t i = 0; i <= n; ++i) {
            const double x = 0.5 + 0.5 * (double)i / (double)n;
            const double r = kCands[c].f(x);
            std::uint64_t u;
            std::memcpy(&u, &r, 8);
            unsigned char b[8];
            for (int k = 0; k < 8; ++k)
                b[k] = (unsigned char)(u >> (8 * k));
            std::fwrite(b, 1, 8, f);
            h = (h ^ u) * 1099511628211ULL;
        }
        std::printf("\"%s\":\"%016llx\"%s", kCands[c].name, (unsigned long long)h,
                    c + 1 < sizeof kCands / sizeof *kCands ? "," : "");
    }
    std::printf("}}\n");
    std::fclose(f);
}
#ifdef RSQRT_HAVE_FRSQRTE
// Reference is plain double 1/sqrt here (IBM long double sqrtl is not accurate enough to trust).
static void frsqrte_mode() {
    struct F {
        const char* n;
        double (*f)(double);
    };
    const F fs[] = {{"frsqrte_raw", frsqrte},
                    {"frsqrte_n2", frsqrte_n2},
                    {"frsqrte_n3", frsqrte_n3},
                    {"B_current", current}};
    std::printf("{\"mode\":\"frsqrte\",\"flags\":\"%s\",\"NOTE\":\"under qemu-user the frsqrte "
                "model is NOT the 7400 hardware table\",\"ranges\":[\n",
                LAB_FLAGS);
    const double ranges[][2] = {{0.5, 1.0}, {0.99, 1.0}, {0.9975, 1.0}};
    for (int r = 0; r < 3; ++r) {
        std::printf(" {\"l2\":[%g,%g]", ranges[r][0], ranges[r][1]);
        for (const F& f : fs) {
            double worst = 0;
            for (int i = 0; i <= 100000; ++i) {
                const double x = ranges[r][0] + (ranges[r][1] - ranges[r][0]) * i / 100000.0;
                const double e = std::fabs(f.f(x) * std::sqrt(x) - 1.0);
                if (e > worst)
                    worst = e;
            }
            std::printf(",\"%s\":%.4g", f.n, worst);
        }
        std::printf("}%s\n", r < 2 ? "," : "");
    }
    std::printf("]}\n");
}
#endif

int main(int argc, char** argv) {
    if (argc >= 2 && !std::strcmp(argv[1], "audit")) {
        audit(argc > 2 ? argv[2] : nullptr);
        return 0;
    }
    if (argc >= 2 && !std::strcmp(argv[1], "bench")) {
        bench(argc > 2 ? argv[2] : nullptr);
        return 0;
    }
    if (argc >= 3 && !std::strcmp(argv[1], "bits")) {
        bits(argv[2]);
        return 0;
    }
#ifdef RSQRT_HAVE_FRSQRTE
    if (argc >= 2 && !std::strcmp(argv[1], "frsqrte")) {
        frsqrte_mode();
        return 0;
    }
#endif
    return 2;
}
