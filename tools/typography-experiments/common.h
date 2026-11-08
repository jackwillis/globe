// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Shared helpers for the host-side typography experiments. Experimental: not part of any build.
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *const kLabels[] = {"San Marino",       "Reykjav\xc3\xadk", "S\xc3\xa3o Paulo",
                                      "Z\xc3\xbcrich",    "C\xc3\xb4te d'Ivoire",
                                      "Mediterranean Sea", "Mississippi River", 0};
enum { kImageW = 260 };

static inline int utf8_next(const char **p) {
    const unsigned char *s = (const unsigned char *)*p;
    int c = *s++;
    if (c >= 0xF0) { c = ((c & 7) << 18) | ((s[0] & 63) << 12) | ((s[1] & 63) << 6) | (s[2] & 63); s += 3; }
    else if (c >= 0xE0) { c = ((c & 15) << 12) | ((s[0] & 63) << 6) | (s[1] & 63); s += 2; }
    else if (c >= 0xC0) { c = ((c & 31) << 6) | (s[0] & 63); s += 1; }
    *p = (const char *)s;
    return c;
}

static inline int label_count(void) { int n = 0; while (kLabels[n]) ++n; return n; }
static inline int image_h(int size) { return label_count() * (size * 2 + 4) + 8; }
static inline int baseline_y(int row, int size) { return 8 + row * (size * 2 + 4) + size + 2; }

static inline void blit_cov(uint8_t *img, int w, int h, int x, int y, const uint8_t *src, int sw, int sh, int pitch) {
    for (int j = 0; j < sh; ++j)
        for (int i = 0; i < sw; ++i) {
            int px = x + i, py = y + j;
            if (px < 0 || py < 0 || px >= w || py >= h) continue;
            int v = src[j * pitch + i];
            if (v > img[py * w + px]) img[py * w + px] = (uint8_t)v;
        }
}

static inline void write_pgm(const char *path, const uint8_t *img, int w, int h) {
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P5\n%d %d\n255\n", w, h);
    fwrite(img, 1, (size_t)w * h, f);
    fclose(f);
}

static inline uint8_t *read_file(const char *path, size_t *n) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END); *n = (size_t)ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t *b = (uint8_t *)malloc(*n);
    if (fread(b, 1, *n, f) != *n) { free(b); b = 0; }
    fclose(f);
    return b;
}

static inline double now_us(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e6 + t.tv_nsec / 1e3;
}
