// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "classic_wait.hpp"
#include <limits>
using namespace globe;
int main() {
    CHECK(classic_wait_ticks(100, 0, false) == 0x7fffffffU);
    CHECK(classic_wait_ticks(100, 0, true) == 0);
    CHECK(classic_wait_ticks(100, 100, false) == 0);
    CHECK(classic_wait_ticks(101, 100, false) == 0);
    CHECK(classic_wait_ticks(100, 101, false) == 1);
    CHECK(classic_wait_ticks(100, 250, false) == 9);
    CHECK(classic_wait_ticks(100, 251, false) == 10);
    CHECK(classic_wait_ticks(0, std::numeric_limits<std::uint64_t>::max(), false) == 0x7fffffffU);
    CHECK(classic_wait_ticks(100, 0, true, false) == 0x7fffffffU);
    CHECK(classic_wait_ticks(100, 1, false, false) == 0x7fffffffU);
    CHECK(classic_wait_ticks(100, 200, true, false) == 0x7fffffffU);
    CHECK(classic_wait_ticks(200, 100, true, true) == 0);
    return TEST_MAIN_RESULT();
}
