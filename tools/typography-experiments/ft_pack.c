// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Prototype C (sizing): how many bytes a pre-rasterized glyph pack would take for a repertoire.
// Usage: ft_pack FONT SIZE   Prints glyph count and byte totals for 1 bit and 8 bit packs.
#include <ft2build.h>
#include FT_FREETYPE_H
#include "common.h"

int main(int argc, char **argv) {
    if (argc < 3) return 2;
    int size = atoi(argv[2]);
    size_t fn = 0; uint8_t *fb = read_file(argv[1], &fn);
    if (!fb) return 1;
    FT_Library lib; FT_Face face;
    if (FT_Init_FreeType(&lib) || FT_New_Memory_Face(lib, fb, (FT_Long)fn, 0, &face)) return 1;
    FT_Set_Pixel_Sizes(face, 0, (FT_UInt)size);
    // Repertoire: ASCII 0x20-0x7E, Latin-1 0xA0-0xFF, Latin Extended-A 0x100-0x17F.
    int ranges[3][2] = {{0x20, 0x7E}, {0xA0, 0xFF}, {0x100, 0x17F}};
    int glyphs = 0; size_t b1 = 0, b8 = 0, missing = 0;
    for (int r = 0; r < 3; ++r)
        for (int c = ranges[r][0]; c <= ranges[r][1]; ++c) {
            FT_UInt gi = FT_Get_Char_Index(face, (FT_ULong)c);
            if (!gi) { ++missing; continue; }
            if (FT_Load_Glyph(face, gi, FT_LOAD_TARGET_MONO) || FT_Render_Glyph(face->glyph, FT_RENDER_MODE_MONO)) continue;
            FT_Bitmap *b = &face->glyph->bitmap;
            b1 += (size_t)((b->width + 7) / 8) * b->rows; b8 += (size_t)b->width * b->rows; ++glyphs;
        }
    // 8 bytes of metrics per glyph (advance, left, top, width, height) plus a 2-byte code in the index.
    printf("{\"size\":%d,\"glyphs\":%d,\"missing_in_font\":%zu,\"bitmap_bytes_1bpp\":%zu,\"bitmap_bytes_8bpp\":%zu,"
           "\"metrics_index_bytes\":%d}\n", size, glyphs, missing, b1, b8, glyphs * 10);
    return 0;
}
