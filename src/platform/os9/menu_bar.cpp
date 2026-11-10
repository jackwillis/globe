// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "menu_bar.hpp"
#include "globe/menus.hpp"
#include "globe/strings.hpp"
#include <Menus.h>
#include <Resources.h>
#include <algorithm>
#include <cstring>
namespace globe {
namespace {
constexpr unsigned char kEllipsis = 0xC9;
constexpr unsigned char kApple = 0x14;
// Appends an item: `text`, an optional ellipsis, an optional command key. Text is plain, so the
// Menu Manager's meta characters are never interpreted.
void add_item(MenuHandle menu, const char* text, char key = 0, bool ellipsis = false) {
    unsigned char line[64];
    unsigned n = std::min<std::size_t>(std::strlen(text), 40);
    std::memcpy(line + 1, text, n);
    if (ellipsis)
        line[++n] = kEllipsis;
    // AppendMenu reads the string for meta characters: add a neutral item, then set its text.
    const unsigned char placeholder[] = {1, 'x'};
    AppendMenu(menu, placeholder);
    const short item = CountMItems(menu);
    line[0] = static_cast<unsigned char>(n);
    SetMenuItemText(menu, item, line);
    if (key)
        SetItemCmd(menu, item, static_cast<short>(key)); // the command-key equivalent
}
void add_divider(MenuHandle menu) {
    const unsigned char divider[] = {2, '(', '-'};
    AppendMenu(menu, divider);
}
MenuHandle make(short id, const unsigned char* title) {
    return NewMenu(id, title);
}
void titled(short id, const char* text, MenuHandle* out) {
    unsigned char title[64];
    const unsigned n = std::min<std::size_t>(std::strlen(text), 30);
    title[0] = static_cast<unsigned char>(n);
    std::memcpy(title + 1, text, n);
    *out = make(id, title);
}
} // namespace
void rebuild_menu_bar(Language lang) {
    for (short id : {kAppleMenu, kFileMenu, kEditMenu, kViewMenu}) {
        if (const MenuHandle old = GetMenuHandle(id)) {
            DeleteMenu(id);
            DisposeMenu(old);
        }
    }
    MenuHandle apple, file, edit, view;
    const unsigned char apple_title[] = {1, kApple};
    apple = make(kAppleMenu, apple_title);
    add_item(apple, tr(lang, Str::AboutTitle), 0, true);
    add_divider(apple);
    titled(kFileMenu, tr(lang, Str::MenuFile), &file);
    add_item(file, tr(lang, Str::ItemQuit), 'Q');
    titled(kEditMenu, tr(lang, Str::MenuEdit), &edit);
    add_item(edit, tr(lang, Str::ItemUndo), 'Z');
    add_divider(edit);
    add_item(edit, tr(lang, Str::ItemCut), 'X');
    add_item(edit, tr(lang, Str::ItemCopy), 'C');
    add_item(edit, tr(lang, Str::ItemPaste), 'V');
    add_item(edit, tr(lang, Str::ItemClear));
    add_divider(edit);
    add_item(edit, tr(lang, Str::ItemPreferences), ',', true);
    titled(kViewMenu, tr(lang, Str::MenuView), &view);
    add_item(view, tr(lang, Str::ItemZoomIn), '+');
    add_item(view, tr(lang, Str::ItemZoomOut), '-');
    add_item(view, tr(lang, Str::ItemResetView), 'R');
    add_item(view, tr(lang, Str::ItemAlignNorth), 'N');
    add_divider(view);
    add_item(view, tr(lang, Str::ItemGraticule), 'G');
    add_item(view, tr(lang, Str::ItemBorders), 'B');
    add_item(view, tr(lang, Str::ItemTrackball), 'T');
    add_divider(view);
    add_item(view, tr(lang, Str::LookMap));
    add_item(view, tr(lang, Str::LookRelief));
    add_divider(view);
    add_item(view, tr(lang, Str::ItemNight), 'D');
    add_item(view, tr(lang, Str::ItemMeasure), 'M');
    for (MenuHandle menu : {apple, file, edit, view})
        InsertMenu(menu, 0);
    AppendResMenu(apple, 'DRVR');
    DrawMenuBar();
}
} // namespace globe
