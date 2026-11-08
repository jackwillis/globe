// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Prototype B: render the test labels with a minimal FreeType build (host).
// Usage: ft_label FONT SIZE MODE OUT.pgm [REPEAT]   MODE: hint-gray | hint-mono | nohint-gray | light-gray
#include <ft2build.h>
#include FT_FREETYPE_H
#include "common.h"
static size_t allocs, frees, cur, peak;
typedef struct { size_t n; size_t pad; } Hdr;
static void *a_alloc(FT_Memory m, long n) { (void)m; Hdr *h = (Hdr *)malloc(sizeof(Hdr) + n); if (!h) return 0; h->n = n; ++allocs; cur += n; if (cur > peak) peak = cur; return h + 1; }
static void a_free(FT_Memory m, void *p) { (void)m; if (!p) return; Hdr *h = (Hdr *)p - 1; ++frees; cur -= h->n; free(h); }
static void *a_realloc(FT_Memory m, long o, long n, void *p) { (void)m; if (!p) return a_alloc(m, n); Hdr *h = (Hdr *)p - 1; Hdr *g = (Hdr *)realloc(h, sizeof(Hdr) + n); if (!g) return 0; cur += n - o; if (cur > peak) peak = cur; g->n = n; return g + 1; }

static int render_all(FT_Face face, int mode_mono, int flags, uint8_t *img, int w, int h, int size) {
    int drawn = 0;
    for (int row = 0; kLabels[row]; ++row) {
        int pen = 6; FT_UInt prev = 0;
        const char *s = kLabels[row];
        while (*s) {
            int cp = utf8_next(&s);
            FT_UInt gi = FT_Get_Char_Index(face, (FT_ULong)cp);
            if (!gi) continue;
            if (prev) { FT_Vector k; if (!FT_Get_Kerning(face, prev, gi, FT_KERNING_DEFAULT, &k)) pen += (int)(k.x >> 6); }
            if (FT_Load_Glyph(face, gi, flags)) continue;
            if (FT_Render_Glyph(face->glyph, mode_mono ? FT_RENDER_MODE_MONO : FT_RENDER_MODE_NORMAL)) continue;
            FT_Bitmap *b = &face->glyph->bitmap;
            int bx = pen + face->glyph->bitmap_left, by = baseline_y(row, size) - face->glyph->bitmap_top;
            if (mode_mono) {
                for (int j = 0; j < (int)b->rows; ++j)
                    for (int i = 0; i < (int)b->width; ++i)
                        if (b->buffer[j * b->pitch + (i >> 3)] & (0x80 >> (i & 7))) {
                            int px = bx + i, py = by + j;
                            if (px >= 0 && py >= 0 && px < w && py < h) img[py * w + px] = 255;
                        }
            } else blit_cov(img, w, h, bx, by, b->buffer, (int)b->width, (int)b->rows, b->pitch);
            long adv = (flags & FT_LOAD_NO_HINTING) ? (face->glyph->linearHoriAdvance + 0x8000) >> 16 : (face->glyph->advance.x + 32) >> 6;
            pen += (int)adv; prev = gi; ++drawn;
        }
    }
    return drawn;
}

int main(int argc, char **argv) {
    if (argc < 5) return 2;
    int size = atoi(argv[2]); const char *mode = argv[3];
    int repeat = argc > 5 ? atoi(argv[5]) : 50;
    size_t fn = 0; uint8_t *fb = read_file(argv[1], &fn);
    if (!fb) return 1;
    struct FT_MemoryRec_ mem = {0, a_alloc, a_free, a_realloc};
    FT_Library lib;
    if (FT_New_Library(&mem, &lib)) return 1;
    FT_Add_Default_Modules(lib);
    FT_Face face;
    if (FT_New_Memory_Face(lib, fb, (FT_Long)fn, 0, &face)) return 1;
    FT_Set_Pixel_Sizes(face, 0, (FT_UInt)size);
    int mono = 0, flags = FT_LOAD_DEFAULT;
    if (!strcmp(mode, "hint-mono")) { mono = 1; flags = FT_LOAD_TARGET_MONO; }
    else if (!strcmp(mode, "nohint-gray")) flags = FT_LOAD_NO_HINTING;
    else if (!strcmp(mode, "light-gray")) flags = FT_LOAD_TARGET_LIGHT | FT_LOAD_FORCE_AUTOHINT;
    int w = kImageW, h = image_h(size);
    uint8_t *img = (uint8_t *)calloc(1, (size_t)w * h);
    size_t a0 = allocs;
    int drawn = render_all(face, mono, flags, img, w, h, size);
    size_t per_pass = allocs - a0;
    double t0 = now_us();
    for (int r = 0; r < repeat; ++r) { memset(img, 0, (size_t)w * h); render_all(face, mono, flags, img, w, h, size); }
    double per_label = (now_us() - t0) / repeat / label_count();
    write_pgm(argv[4], img, w, h);
    printf("{\"renderer\":\"freetype\",\"mode\":\"%s\",\"size\":%d,\"glyphs_drawn\":%d,\"us_per_label\":%.1f,"
           "\"allocs_per_pass\":%zu,\"peak_heap_bytes\":%zu,\"font_file_bytes\":%zu}\n",
           mode, size, drawn, per_label, per_pass, peak, fn);
    return 0;
}
