// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "classic_window_fit.hpp"
#include <cstdint>
#include <limits>
using namespace globe;
int main() {
    constexpr std::int64_t reserve = 1024 * 1024;
    constexpr int sidebar = 220;
    // #106: this globe exactly fits. Charging the sidebar shrank it twice to 480x327.
    constexpr auto exact_bytes = reserve + std::int64_t{408} * 428 * 11;
    const auto exact = fit_classic_window_to_heap(628, 428, sidebar, exact_bytes);
    CHECK(exact.width == 628 && exact.height == 428);
    const auto less = fit_classic_window_to_heap(628, 428, sidebar, exact_bytes - 1);
    CHECK(less.width < 628 && less.height < 428);
    CHECK(std::int64_t{less.width - sidebar} * less.height * 11 <= exact_bytes - 1 - reserve);
    for (auto free_bytes : {std::numeric_limits<std::int64_t>::min(), std::int64_t{-1},
                            std::int64_t{0}, reserve - 1, reserve}) {
        const auto minimum = fit_classic_window_to_heap(1600, 1200, sidebar, free_bytes);
        CHECK(minimum.width == 476 && minimum.height == 256);
    }
    for (const auto input : {ClassicWindowSize{477, 257}, ClassicWindowSize{629, 429},
                             ClassicWindowSize{1600, 1200}}) {
        int last_width = 0, last_height = 0;
        for (std::int64_t budget = 0; budget <= 24 * 1024 * 1024; budget += 8191) {
            const auto got = fit_classic_window_to_heap(input.width, input.height, sidebar, budget);
            CHECK(got.width >= 476 && got.height >= 256);
            CHECK(got.width <= input.width && got.height <= input.height);
            CHECK(got.width >= last_width && got.height >= last_height);
            const auto cost = std::int64_t{got.width - sidebar} * got.height * 11;
            CHECK((got.width == 476 && got.height == 256) || cost <= budget - reserve);
            last_width = got.width;
            last_height = got.height;
        }
    }
    const auto maximum =
        fit_classic_window_to_heap(1600, 1200, sidebar, std::numeric_limits<std::int64_t>::max());
    CHECK(maximum.width == 1600 && maximum.height == 1200);
    const auto invalid = fit_classic_window_to_heap(std::numeric_limits<int>::min(),
                                                    std::numeric_limits<int>::min(), sidebar, 0);
    CHECK(invalid.width == 476 && invalid.height == 256);
    return TEST_MAIN_RESULT();
}
