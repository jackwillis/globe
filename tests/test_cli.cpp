// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "cli.hpp"

using namespace globe;

int main() {
    int v = 99;
    CHECK(parse_int_arg("0", 0, 10, v) && v == 0);
    CHECK(parse_int_arg("10", 0, 10, v) && v == 10);
    CHECK(parse_int_arg("-5", -10, 10, v) && v == -5);
    v = 99;
    const char* bad[] = {"",
                         " 1",
                         "1 ",
                         "+1",
                         "1x",
                         "x1",
                         "1.5",
                         "0x10",
                         "11",
                         "-1",
                         "--1",
                         "-",
                         "99999999999999999999",
                         "1e2"};
    for (const char* s : bad) {
        if (parse_int_arg(s, 0, 10, v)) {
            CHECK(false);
        }
    }
    CHECK(v == 99); // failures leave the output alone
    CHECK(!parse_int_arg(nullptr, 0, 10, v));
    CHECK(!parse_int_arg("5", 10, 0, v));                  // empty range
    CHECK(!parse_int_arg("-3", -2, 10, v));                // below the range
    CHECK(!parse_int_arg("2147483648", 0, 2147483647, v)); // beyond int

    int w = 7;
    int h = 8;
    CHECK(parse_size_arg("640x480", 4096, w, h) && w == 640 && h == 480);
    CHECK(parse_size_arg("1x1", 4096, w, h) && w == 1 && h == 1);
    CHECK(parse_size_arg("4096x4096", 4096, w, h) && w == 4096);
    w = 7;
    h = 8;
    const char* bad_sizes[] = {"",        "x",        "640",      "640x",
                               "x480",    "0x480",    "640x0",    "-1x5",
                               "5x-1",    "4097x5",   "5x4097",   "640x480x2",
                               "640X480", " 640x480", "640x480 ", "1234567890123456789x5",
                               "6 40x480"};
    for (const char* s : bad_sizes) {
        if (parse_size_arg(s, 4096, w, h)) {
            CHECK(false);
        }
    }
    CHECK(w == 7 && h == 8);
    CHECK(!parse_size_arg(nullptr, 4096, w, h));
    return TEST_MAIN_RESULT();
}
