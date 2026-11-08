// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "classic_present.hpp"
#include <array>
using namespace globe;
int main() {
    ClassicPresentation q;
    std::array<int, 65> frame{}, snapshot{}, screen{};
    frame.fill(1);
    q.request(1);
    CHECK(q.begin(1, 65));
    snapshot = frame;
    // A newer image arrives while the first immutable snapshot is being submitted.
    frame.fill(2);
    q.request(1);
    CHECK(!q.begin(1, 65));
    unsigned bands = 0;
    while (q.active(1)) {
        CHECK(q.end() - q.first() <= 32);
        for (int y = q.first(); y < q.end(); ++y)
            screen[y] = snapshot[y];
        q.submitted(1);
        ++bands;
    }
    CHECK(bands == 3);
    for (auto p : screen)
        CHECK(p == 1);
    CHECK(q.needs_capture(1));
    snapshot = frame;
    CHECK(q.begin(1, 65));
    while (q.active(1)) {
        for (int y = q.first(); y < q.end(); ++y)
            screen[y] = snapshot[y];
        q.submitted(1);
    }
    CHECK(!q.pending(1));
    for (auto p : screen)
        CHECK(p == 2);
    q.request(2);
    CHECK(!q.begin(1, 65));
    CHECK(!q.begin(2, 0));
    CHECK(q.begin(2, 65));
    q.request(3); // new camera invalidates unfinished old rows
    CHECK(!q.active(2));
    q.submitted(2);
    CHECK(q.needs_capture(3));
    CHECK(q.begin(3, 65));
    CHECK(q.first() == 0);
    q.cancel(); // resize/disposal
    CHECK(!q.pending(3));
    // Slice height follows the machine. Start at 32 rows; fast copies at most double it each time
    // (up to the whole frame), slow ones shrink it (never below 8); the slices still cover every
    // row once.
    {
        ClassicPresentation t;
        CHECK(t.rows_per_slice() == 32);
        t.request(1);
        CHECK(t.begin(1, 480));
        CHECK(t.end() - t.first() == 32);
        t.tune(32, 0, 6); // instant: double
        CHECK(t.rows_per_slice() == 64);
        t.tune(64, 1, 6); // 1 ms for 64 rows would allow 384: capped at twice
        CHECK(t.rows_per_slice() == 128);
        t.tune(128, 3, 6); // exactly on budget scaled: 256 allowed, doubled cap 256
        CHECK(t.rows_per_slice() == 256);
        t.tune(256, 12, 6); // too slow: halve
        CHECK(t.rows_per_slice() == 128);
        t.tune(128, 1000, 6); // very slow: the floor
        CHECK(t.rows_per_slice() == 8);
        t.tune(0, 5, 6);
        t.tune(8, 5, 0); // ignored
        CHECK(t.rows_per_slice() == 8);
        int covered = 0;
        for (int i = 0; i < 4; ++i) {
            t.tune(t.rows_per_slice(), 0, 6);
        }
        CHECK(t.rows_per_slice() == 128);
        while (t.active(1)) {
            CHECK(t.first() == covered);
            covered = t.end();
            t.submitted(1);
        }
        CHECK(covered == 480);
        // A slice never runs past the frame, however tall.
        ClassicPresentation big;
        for (int i = 0; i < 12; ++i)
            big.tune(big.rows_per_slice(), 0, 6);
        CHECK(big.rows_per_slice() == 4096);
        big.request(1);
        CHECK(big.begin(1, 300) && big.first() == 0 && big.end() == 300);
        big.submitted(1);
        CHECK(!big.active(1));
    }
    return TEST_MAIN_RESULT();
}
