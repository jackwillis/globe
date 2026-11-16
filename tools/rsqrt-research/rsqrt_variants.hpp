// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Research code for #145. Candidate implementations of 1/sqrt(l2) for the chord-normalization
// helper in src/render/render.cpp. Nothing here is linked into the product. Each function states
// its declared domain; outside it the function falls back to the accurate path, never to a
// silently wider error.
#pragma once
#include <cmath>

namespace rsqrt_lab {

// A: accurate reference.
inline double exact(double x) {
    return 1.0 / std::sqrt(x);
}

// B: the helper exactly as it is on main (0b5b16d): two Newton steps from 1 for l2 >= 0.5.
inline double current(double l2) {
    if (l2 < 0.5)
        return 1.0 / std::sqrt(l2);
    double s = 1.5 - 0.5 * l2;
    s *= 1.5 - 0.5 * l2 * s * s;
    return s;
}

// B without its exact fallback, for studying the Newton map itself on any input.
inline double current_raw(double l2) {
    double s = 1.5 - 0.5 * l2;
    s *= 1.5 - 0.5 * l2 * s * s;
    return s;
}

// B2: the current helper with its exact-path guard moved from 0.5 to 0.92, the point where the
// two-step error first drops below the 1e-5 the original comment promises (about 33 degrees of arc).
// Identical arithmetic for every arc the shipped data can produce; arcs longer than ~33 degrees take
// the accurate path (slow on a G4, but no such arc is drawn today).
inline double guarded(double l2) {
    if (l2 < 0.92)
        return 1.0 / std::sqrt(l2);
    double s = 1.5 - 0.5 * l2;
    s *= 1.5 - 0.5 * l2 * s * s;
    return s;
}

// C1: one Newton step from 1, declared domain l2 >= 0.99 (arcs up to about 16 degrees).
// Relative error is e = 1.5 * (1 - sqrt(l2))^2 approximately, at most about 1.5e-4 at 0.99 and
// about 2.3e-6 at the shipped-data limit l2 = 0.9975.
inline double newton1(double l2) {
    if (!(l2 >= 0.99))
        return current(l2);
    return 1.5 - 0.5 * l2;
}

// C2: Taylor series of (1 - d)^(-1/2) in d = 1 - l2 through d^3. Error is 35/128 d^4 (about
// 1.1e-11 at d = 0.0025). Declared domain l2 >= 0.99; no Newton step, no multiply chain.
inline double taylor3(double l2) {
    if (!(l2 >= 0.99))
        return current(l2);
    const double d = 1.0 - l2;
    return 1.0 + d * (0.5 + d * (0.375 + d * 0.3125));
}

// C4: honours the comment's "two Newton steps, small error" on the WHOLE documented domain
// [0.5, 1]: a linear minimax seed (relative error 2.7%) and two full Newton steps. Costs about
// five more flops than the current helper; bound about 1.9e-6.
inline double seeded2(double l2) {
    if (!(l2 >= 0.5))
        return 1.0 / std::sqrt(l2);
    double s = 1.7560 - 0.7604 * l2;
    s *= 1.5 - 0.5 * l2 * s * s;
    s *= 1.5 - 0.5 * l2 * s * s;
    return s;
}

} // namespace rsqrt_lab
