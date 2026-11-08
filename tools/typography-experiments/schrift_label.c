// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Prototype A: render the test labels with libschrift (host). Usage: schrift_label FONT SIZE OUT.pgm [REPEAT]
#include "common.h"
#include "schrift.h"
extern size_t cnt_allocs, cnt_frees, cnt_cur, cnt_peak;

static int render_all(SFT *sft, uint8_t *img, int w, int h, int size) {
    int drawn = 0;
    for (int row = 0; kLabels[row]; ++row) {
        double pen = 6; SFT_Glyph prev = 0; int have_prev = 0;
        const char *s = kLabels[row];
        while (*s) {
            int cp = utf8_next(&s);
            SFT_Glyph g;
            if (sft_lookup(sft, (SFT_UChar)cp, &g) < 0) continue;
            SFT_GMetrics m;
            if (sft_gmetrics(sft, g, &m) < 0) continue;
            if (have_prev) { SFT_Kerning k; if (sft_kerning(sft, prev, g, &k) == 0) pen += k.xShift; }
            if (m.minWidth > 0 && m.minHeight > 0) {
                uint8_t *px = (uint8_t *)calloc(1, (size_t)m.minWidth * m.minHeight);
                SFT_Image im = {px, m.minWidth, m.minHeight};
                if (sft_render(sft, g, im) == 0)
                    blit_cov(img, w, h, (int)(pen + m.leftSideBearing + 0.5), baseline_y(row, size) + m.yOffset, px, m.minWidth, m.minHeight, m.minWidth);
                free(px);
            }
            pen += m.advanceWidth; prev = g; have_prev = 1; ++drawn;
        }
    }
    return drawn;
}

int main(int argc, char **argv) {
    if (argc < 4) return 2;
    int size = atoi(argv[2]), repeat = argc > 4 ? atoi(argv[4]) : 50;
    size_t fn = 0; uint8_t *fb = read_file(argv[1], &fn);
    if (!fb) return 1;
    SFT_Font *font = sft_loadmem(fb, fn);
    if (!font) return 1;
    SFT sft = {font, size, size, 0, 0, SFT_DOWNWARD_Y};
    int w = kImageW, h = image_h(size);
    uint8_t *img = (uint8_t *)calloc(1, (size_t)w * h);
    size_t a0 = cnt_allocs; cnt_peak = cnt_cur;
    int drawn = render_all(&sft, img, w, h, size);
    size_t allocs_per_pass = cnt_allocs - a0; size_t peak = cnt_peak;
    double t0 = now_us();
    for (int r = 0; r < repeat; ++r) { memset(img, 0, (size_t)w * h); render_all(&sft, img, w, h, size); }
    double per_label = (now_us() - t0) / repeat / label_count();
    write_pgm(argv[3], img, w, h);
    printf("{\"renderer\":\"libschrift\",\"size\":%d,\"glyphs_drawn\":%d,\"us_per_label\":%.1f,"
           "\"allocs_per_pass\":%zu,\"peak_heap_bytes\":%zu,\"font_file_bytes\":%zu}\n",
           size, drawn, per_label, allocs_per_pass, peak, fn);
    return 0;
}
