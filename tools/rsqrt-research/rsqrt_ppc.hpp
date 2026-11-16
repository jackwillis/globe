// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Research code for #145: PowerPC reciprocal-square-root estimate candidates (treatment D).
// Compiled only when the COMPILER says the target has the PowerPC graphics group
// (_ARCH_PPCGR: fres, frsqrte, fsel). That macro is set by -mcpu=603e/604/750/7400/7450 and NOT by
// the default generic -mcpu=powerpc, so this never turns on by __powerpc__ alone.
#pragma once
#if defined(_ARCH_PPCGR) && defined(__powerpc__)
#define RSQRT_HAVE_FRSQRTE 1
namespace rsqrt_lab {
inline double frsqrte(double x) {
    double r;
    asm("frsqrte %0,%1" : "=f"(r) : "f"(x));
    return r;
}
// D2/D3: hardware estimate (guaranteed only to ~1/32 relative error) plus 2 or 3 Newton steps.
// 3 steps are needed to reach ~1e-11, which is what the existing 2-step-from-1 helper achieves on
// the shipped-data domain.
inline double frsqrte_n2(double x) {
    double s = frsqrte(x);
    s *= 1.5 - 0.5 * x * s * s;
    s *= 1.5 - 0.5 * x * s * s;
    return s;
}
inline double frsqrte_n3(double x) {
    double s = frsqrte(x);
    s *= 1.5 - 0.5 * x * s * s;
    s *= 1.5 - 0.5 * x * s * s;
    s *= 1.5 - 0.5 * x * s * s;
    return s;
}
} // namespace rsqrt_lab
#endif
