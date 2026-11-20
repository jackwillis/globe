// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "script_output.hpp"
using namespace globe;
struct Output {
    short* reference;
    bool close_ok, flush_ok;
    unsigned calls = 0;
    static bool close(void* p, short ref) {
        auto& s = *static_cast<Output*>(p);
        CHECK(ref == 123);
        CHECK(*s.reference == -1);
        CHECK(s.calls++ == 0);
        return s.close_ok;
    }
    static bool flush(void* p, short volume) {
        auto& s = *static_cast<Output*>(p);
        CHECK(volume == -7);
        CHECK(s.calls++ == 1);
        return s.flush_ok;
    }
};
int main() {
    for (unsigned failures = 0; failures < 4; ++failures) {
        short ref = 123;
        Output output{&ref, (failures & 1) == 0, (failures & 2) == 0};
        const auto result = script_close_output(ref, -7, Output::close, Output::flush, &output);
        CHECK(result.close_failed == !output.close_ok);
        CHECK(result.flush_failed == !output.flush_ok);
        CHECK(ref == -1 && output.calls == 2);
        const auto again = script_close_output(ref, -7, Output::close, Output::flush, &output);
        CHECK(!again.close_failed && !again.flush_failed && output.calls == 2);
    }
    return TEST_MAIN_RESULT();
}
