// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Research code for #145: one non-inlined function per candidate so the emitted PowerPC code can be
// read and counted (see ppc_asm_report.py). Compile-only evidence; nothing here runs on a G4.
#include "rsqrt_ppc.hpp"
#include "rsqrt_variants.hpp"
using namespace rsqrt_lab;
extern "C" {
double f_exact(double x) {
    return exact(x);
}
double f_current(double x) {
    return current(x);
}
double f_current_raw(double x) {
    return current_raw(x);
}
double f_newton1(double x) {
    return newton1(x);
}
double f_taylor3(double x) {
    return taylor3(x);
}
double f_seeded2(double x) {
    return seeded2(x);
}
// Fast-path cores only (no domain test, no fallback call): what the hot loop would execute.
double f_core_current(double x) {
    return current_raw(x);
}
double f_core_newton1(double x) {
    return 1.5 - 0.5 * x;
}
double f_core_taylor3(double x) {
    const double d = 1.0 - x;
    return 1.0 + d * (0.5 + d * (0.375 + d * 0.3125));
}
double f_core_seeded2(double x) {
    double s = 1.7560 - 0.7604 * x;
    s *= 1.5 - 0.5 * x * s * s;
    s *= 1.5 - 0.5 * x * s * s;
    return s;
}
#ifdef RSQRT_HAVE_FRSQRTE
double f_frsqrte_raw(double x) {
    return frsqrte(x);
}
double f_frsqrte_n2(double x) {
    return frsqrte_n2(x);
}
double f_frsqrte_n3(double x) {
    return frsqrte_n3(x);
}
#endif
}
