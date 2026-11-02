// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Minimal test harness: no framework dependency, no exceptions, runs on any target.
#pragma once

#include <cmath>
#include <cstdio>

namespace test {

inline int& failures() {
    static int count = 0;
    return count;
}

} // namespace test

#define CHECK(cond)                                                                              \
    do {                                                                                         \
        if (!(cond)) {                                                                           \
            std::printf("%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond);                 \
            ++test::failures();                                                                  \
        }                                                                                        \
    } while (0)

#define CHECK_NEAR(a, b, tol)                                                                    \
    do {                                                                                         \
        const double check_a = (a);                                                              \
        const double check_b = (b);                                                              \
        if (!(std::fabs(check_a - check_b) <= (tol))) {                                          \
            std::printf("%s:%d: CHECK_NEAR failed: %s = %.17g, %s = %.17g\n", __FILE__,          \
                        __LINE__, #a, check_a, #b, check_b);                                     \
            ++test::failures();                                                                  \
        }                                                                                        \
    } while (0)

#define TEST_MAIN_RESULT()                                                                       \
    (test::failures() == 0 ? (std::printf("ok\n"), 0)                                            \
                           : (std::printf("%d failure(s)\n", test::failures()), 1))
