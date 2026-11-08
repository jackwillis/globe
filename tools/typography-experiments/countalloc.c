// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include <stdlib.h>
#include <string.h>
size_t cnt_allocs, cnt_frees, cnt_cur, cnt_peak;
typedef struct { size_t n; size_t pad; } Hdr;
static void note(size_t add, size_t sub) {
    cnt_cur = cnt_cur + add - sub;
    if (cnt_cur > cnt_peak) cnt_peak = cnt_cur;
}
void *cnt_malloc(size_t n) {
    Hdr *h = (Hdr *)malloc(sizeof(Hdr) + n);
    if (!h) return 0;
    h->n = n; ++cnt_allocs; note(n, 0);
    return h + 1;
}
void *cnt_calloc(size_t a, size_t b) {
    void *p = cnt_malloc(a * b);
    if (p) memset(p, 0, a * b);
    return p;
}
void cnt_free(void *p) {
    if (!p) return;
    Hdr *h = (Hdr *)p - 1;
    ++cnt_frees; note(0, h->n);
    free(h);
}
void *cnt_realloc(void *p, size_t n) {
    if (!p) return cnt_malloc(n);
    Hdr *h = (Hdr *)p - 1;
    size_t old = h->n;
    Hdr *g = (Hdr *)realloc(h, sizeof(Hdr) + n);
    if (!g) return 0;
    g->n = n; note(n, old);
    return g + 1;
}
