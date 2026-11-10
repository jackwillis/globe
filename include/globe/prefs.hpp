// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// User preferences: part of Model, changed only through update() (SetPreferences).
#pragma once
namespace globe {
enum class Units : unsigned char { Metric, Imperial };
enum class Language : unsigned char { English, Spanish };
constexpr unsigned kLanguageCount = 2;
struct Preferences {
    Units units = Units::Metric;
    Language language = Language::English;
    friend bool operator==(const Preferences&, const Preferences&) = default;
};
} // namespace globe
