// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis

#include "check.hpp"
#include "globe/version.hpp"

int main(int argc, char** argv) {
    CHECK(argc == 2);
    if (argc == 2)
        CHECK(globe::kVersion == argv[1]);

    unsigned components = 0;
    bool has_digits = false;
    for (const char ch : globe::kVersion) {
        if (ch == '.') {
            CHECK(has_digits);
            ++components;
            has_digits = false;
        } else {
            CHECK(ch >= '0' && ch <= '9');
            has_digits = true;
        }
    }
    CHECK(has_digits && components == 2);
    return TEST_MAIN_RESULT();
}
