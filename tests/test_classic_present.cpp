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
    return TEST_MAIN_RESULT();
}
