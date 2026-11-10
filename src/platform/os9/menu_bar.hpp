// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Rebuilds the menu bar in a language (the Rez menus are the English start-up bar). Menu ids and
// item numbers are those of include/globe/menus.hpp.
#pragma once
#include "globe/prefs.hpp"
namespace globe {
void rebuild_menu_bar(Language language);
} // namespace globe
