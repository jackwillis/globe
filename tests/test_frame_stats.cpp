// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include <vector>

#include "check.hpp"
#include "clock.hpp"
#include "frame_stats.hpp"

using namespace globe;

int main() {
    Summary z = summarize(nullptr, 0);
    CHECK(z.count == 0 && z.max == 0.0 && z.mean == 0.0);
    double none[1] = {5.0};
    CHECK(summarize(none, 0).count == 0);

    double one[] = {7.0};
    Summary s1 = summarize(one, 1);
    CHECK(s1.count == 1 && s1.min == 7.0 && s1.max == 7.0 && s1.mean == 7.0 && s1.p50 == 7.0 &&
          s1.p95 == 7.0);

    // 1..100 in scrambled order: nearest-rank p50 = 50, p95 = 95.
    std::vector<double> v;
    for (int i = 0; i < 100; ++i) {
        v.push_back(static_cast<double>((i * 37) % 100 + 1));
    }
    Summary s = summarize(v.data(), v.size());
    CHECK(s.count == 100 && s.min == 1.0 && s.max == 100.0);
    CHECK_NEAR(s.mean, 50.5, 1e-12);
    CHECK(s.p50 == 50.0 && s.p95 == 95.0);

    // 10 samples: ceil(0.95 * 10) = 10 -> the maximum; ceil(0.5 * 10) = 5th value.
    double ten[] = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    Summary t = summarize(ten, 10);
    CHECK(t.p50 == 5.0 && t.p95 == 10.0 && t.min == 1.0);
    // Sorted in place, as documented.
    CHECK(ten[0] == 1.0 && ten[9] == 10.0);

    // Duplicates and an outlier: the mean moves, the median does not.
    double d[] = {2, 2, 2, 2, 100};
    Summary ds = summarize(d, 5);
    CHECK(ds.p50 == 2.0 && ds.max == 100.0 && ds.mean == 21.6);

    // The clock never goes backwards and advances.
    std::uint64_t prev = monotonic_ns();
    bool moved = false;
    for (int i = 0; i < 100000; ++i) {
        const std::uint64_t now = monotonic_ns();
        CHECK(now >= prev);
        moved |= now > prev;
        prev = now;
    }
    CHECK(moved);
    return TEST_MAIN_RESULT();
}
