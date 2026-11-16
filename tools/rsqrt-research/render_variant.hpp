// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Scratch-build hook: render.cpp is compiled with -DGLOBE_RSQRT_VARIANT=<n> and this header on the
// include path (see apply_variant.sh). Variant 0 is the helper unchanged; 5 also logs every
// argument so the true production domain can be measured. The repository itself is not modified.
#pragma once
#include <cstdio>
#include "rsqrt_variants.hpp"
namespace rsqrt_lab {
inline std::FILE* g_log = nullptr; // argument log, opened by the trace tool
inline unsigned long g_calls = 0;  // number of inverse_length calls
inline double g_radius = 0;
inline unsigned long g_steps_sqrt = 0; // draw_arc arcs that took the std::sqrt step-count branch        // radius of the frame being drawn, set by the trace tool
inline double dispatch(double l2) {
    ++g_calls;
#if GLOBE_RSQRT_VARIANT == 1
    return exact(l2);
#elif GLOBE_RSQRT_VARIANT == 2
    return newton1(l2);
#elif GLOBE_RSQRT_VARIANT == 3
    return taylor3(l2);
#elif GLOBE_RSQRT_VARIANT == 4
    return seeded2(l2);
#elif GLOBE_RSQRT_VARIANT == 6
    return guarded(l2);
#elif GLOBE_RSQRT_VARIANT == 5
    if (g_log) { std::fwrite(&l2, sizeof l2, 1, g_log); std::fwrite(&g_radius, sizeof g_radius, 1, g_log); }
    return current(l2);
#else
    return current(l2);
#endif
}
} // namespace rsqrt_lab
