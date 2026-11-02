// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Strict command-line number parsing shared by the platform programs: the whole string must be
// a number inside the range, with no sign tricks, spaces or trailing text.
#pragma once

namespace globe {

// Decimal integer in [lo, hi]. A leading '-' is accepted only if lo < 0; '+' and whitespace are
// not accepted. Leaves `out` unchanged on failure.
bool parse_int_arg(const char* text, int lo, int hi, int& out);

// "WIDTHxHEIGHT" with both parts positive and at most `max_side`. Leaves outputs unchanged on
// failure.
bool parse_size_arg(const char* text, int max_side, int& width, int& height);

} // namespace globe
