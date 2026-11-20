// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "script_text.hpp"
#include <cstring>
#include <limits>
using namespace globe;
int main() {
    char text[256];
    CHECK(script_filename("long-long-long.script", 0x1234abcd, text, sizeof text));
    CHECK(std::strcmp(text, "long-long--1234abcd.jsonl") == 0);
    CHECK(script_filename(".script", 0, text, sizeof text));
    CHECK(std::strcmp(text, "script-00000000.jsonl") == 0);
    CHECK(script_filename("a:b \x8e.script", 0xffffffff, text, sizeof text));
    CHECK(std::strcmp(text, "a_b__-ffffffff.jsonl") == 0);
    CHECK(script_os_version(0x0922, text, sizeof text));
    CHECK(std::strcmp(text, "Mac OS 9.2.2") == 0);
    CHECK(script_os_version(0x1030, text, sizeof text));
    CHECK(std::strcmp(text, "Mac OS 10.3.0") == 0);
    CHECK(script_progress_title(std::numeric_limits<unsigned>::max(), text, sizeof text));
    CHECK(std::strcmp(text, "Poodle Earth - Script command 4294967295 (Esc cancels)") == 0);
    CHECK(script_summary({"pass", 4, 0, 0, "none", true, false}, text, sizeof text));
    CHECK(
        std::strcmp(
            text,
            "Script pass. Commands: 4; assertions failed: 0; errors: 0. none. Transcript saved") ==
        0);
    CHECK(script_summary({"error", 4, 0, 1, "flush-failure", true, true}, text, sizeof text));
    CHECK(std::strstr(text, "flush-failure. Transcript NOT saved successfully"));
    CHECK(script_summary({"cancelled", 1, 0, 0, "cancelled", false, false}, text, sizeof text));
    CHECK(std::strstr(text, "Output saving disabled"));
    char tiny[2] = {'x', 'x'};
    CHECK(!script_progress_title(1, tiny, sizeof tiny));
    CHECK(tiny[1] == 0);
    CHECK(!script_filename("a", 0, tiny, 0));
    CHECK(tiny[0] == 'P');
    return TEST_MAIN_RESULT();
}
