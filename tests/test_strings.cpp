// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/strings.hpp"
#include <cstring>
using namespace globe;
int main() {
    for (unsigned lang = 0; lang < kLanguageCount; ++lang)
        for (unsigned i = 0; i < static_cast<unsigned>(Str::Count); ++i) {
            const char* text = tr(static_cast<Language>(lang), static_cast<Str>(i));
            CHECK(text != nullptr && text[0] != '\0');
        }
    CHECK(std::strcmp(tr(Language::English, Str::LookRelief), "Relief") == 0);
    CHECK(std::strcmp(tr(Language::Spanish, Str::LookRelief), "Relieve") == 0);
    CHECK(tr(Language::English, Str::Count)[0] == '\0');
    return TEST_MAIN_RESULT();
}
