// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Counting allocator for the experiments: include before the code under test (-include).
#pragma once
#include <stddef.h>
#include <stdlib.h>
void *cnt_malloc(size_t n);
void *cnt_calloc(size_t a, size_t b);
void *cnt_realloc(void *p, size_t n);
void cnt_free(void *p);
extern size_t cnt_allocs, cnt_frees, cnt_cur, cnt_peak;
#define malloc cnt_malloc
#define calloc cnt_calloc
#define realloc cnt_realloc
#define free cnt_free
