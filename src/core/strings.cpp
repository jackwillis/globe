// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/strings.hpp"
namespace globe {
namespace {
constexpr const char* kEnglish[] = {
#include "strings_en.inc"
};
constexpr unsigned kCount = static_cast<unsigned>(Str::Count);
static_assert(sizeof kEnglish / sizeof *kEnglish == kCount, "en.txt is out of date");
} // namespace
const char* tr(Str key) {
    const auto i = static_cast<unsigned>(key);
    if (i >= kCount)
        return "";
    return kEnglish[i];
}
} // namespace globe
