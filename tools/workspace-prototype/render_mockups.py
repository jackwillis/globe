#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Renders the #148 workspace mockups and the card-fit table (host only, no Toolbox).

Window rectangles come from layout_dump.cpp, which includes the tested geometry header; card text is
wrapped with the same greedy word algorithm as include/globe/wrap.hpp, measured with an installed
font. Chrome, glyph shapes and the globe picture are illustrative: this is not what Mac OS 9 draws.
All fact numbers are MOCK DATA and are labelled as such inside each card.

Usage: render_mockups.py [output-dir]      (default: docs/design/images)
Needs Pillow and a C++20 compiler. Fonts are looked up with fc-match, never bundled.
"""
import pathlib
import subprocess
import sys
import tempfile

from PIL import Image, ImageDraw, ImageFont

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parents[1]
OUT = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "docs/design/images"

# Sidebar constants from src/platform/os9/sidebar.cpp.
LINE, HEAD, MARGIN, BAR, SIDEBAR_W, ROW, LOOKS = 14, 22, 8, 16, 220, 22, 2
# Width stand-in for the system font (Chicago/Charcoal are wider than Helvetica at 12): glyph
# advances are multiplied by this factor and rounded to whole pixels, as Quickdraw would.
FACTORS = {"Helvetica-like": 1.0, "wide system font (assumed)": 1.2, "very wide": 1.4}
DEFAULT_FACTOR = 1.2


def font_path(pattern):
    return subprocess.run(["fc-match", "-f", "%{file}", pattern], capture_output=True, text=True,
                          check=True).stdout


FONT = ImageFont.truetype(font_path("Liberation Sans"), 12)
BOLD = ImageFont.truetype(font_path("Liberation Sans:bold"), 12)


def advance(ch, font, factor):
    return max(1, round(font.getlength(ch) * factor))


def text_width(text, factor=DEFAULT_FACTOR, font=FONT):
    return sum(advance(c, font, factor) for c in text)


def draw_text(d, x, baseline, text, font=FONT, factor=DEFAULT_FACTOR, fill=(0, 0, 0)):
    for c in text:
        d.text((x, baseline), c, font=font, fill=fill, anchor="ls")
        x += advance(c, font, factor)
    return x


# ---- the content contract: title plus up to 21 plain lines (include/globe/countries.hpp) ------
class Card:
    def __init__(self, title, lines):
        assert len(lines) <= 21
        self.title, self.lines = title, lines


CARDS = {
    "city": Card("Madison", [
        "City in Wisconsin, United States",
        "Population: 269,840 (2020 census, city proper)",
        "Location: 43.07 N, 89.40 W",
        "Part of: Dane County, Wisconsin",
        "Source: Wikidata statement, retrieved 2026-11-01",
        "MOCK DATA for layout study; not verified.",
    ]),
    "ambiguous": Card("Paris", [
        "City in France",
        "Population: not shown. The source lists 3 values for different areas (city, urban area, "
        "metro area); none is reviewed.",
        "Area: not available",
        "Location: 48.86 N, 2.35 E",
        "Source: Wikidata (mock fixture), retrieved 2026-11-01",
        "MOCK DATA for layout study; not verified.",
    ]),
    "country": Card("Peru", [
        "Country",
        "Population: 33,000,000 (2023 estimate, resident population)",
        "Area: 1,285,000 km2 (land and inland water)",
        "Capital: Lima",
        "Region: South America",
        "Source: UN and Natural Earth (mock fixture), retrieved 2026-11-01",
        "MOCK DATA for layout study; not verified.",
    ]),
    # Verbatim from src/core/countries.cpp country_card() on origin/main 0b5b16d.
    "today": Card("Peru", [
        "Country or territory", "Natural Earth 1:110m / 5.1.1", "Public domain geographic data",
        "Generalized boundaries; islands omitted.", "Names and disputed areas follow",
        "Natural Earth's source records.", "naturalearthdata.com"]),
    # A later Inspector-style card (#137 comment): headings, qualified facts, links as plain text.
    "stress": Card("Madison", (["City in Wisconsin, United States", "Details"] + [
        "Population: 269,840 (2020 census, city proper)", "Elevation: 262 m (mock)",
        "Location: 43.07 N, 89.40 W", "Explore", "Part of: Dane County; Wisconsin; United States",
        "Nearby: Middleton; Fitchburg; Sun Prairie; Verona", "Neighbors: 4 reviewed",
        "Sources and identity", "Source: Wikidata statement, retrieved 2026-11-01",
        "Identity: reviewed", "Licence: CC0 (mock)", "MOCK DATA for layout study."])),
}


def wrap(card, max_width, factor=DEFAULT_FACTOR):
    """Mirror of include/globe/wrap.hpp: greedy, at spaces, over-long words broken by character."""
    rows = []
    for line in card.lines:
        rest = line
        if not rest:
            rows.append("")
        while rest:
            while len(rest) > 1 and rest[0] == " ":
                rest = rest[1:]
            fit = end = 0
            while end < len(rest):
                nxt = rest.find(" ", end)
                nxt = len(rest) if nxt == -1 else nxt
                if text_width(rest[:nxt], factor) > max_width:
                    break
                fit, end = nxt, nxt + 1
            if fit == 0:
                fit = 1
                while fit < len(rest) and text_width(rest[:fit + 1], factor) <= max_width:
                    fit += 1
            rows.append(rest[:fit])
            rest = rest[fit:]
    return rows[:48]  # WrappedCard::kCapacity


def visible_rows(height):
    return max(1, (height - HEAD - 4) // LINE)


def rows_needed(card, pane_w, factor=DEFAULT_FACTOR):
    return len(wrap(card, pane_w - MARGIN - BAR - 4, factor))


# ---- illustrative Classic chrome --------------------------------------------------------------
DESK, WHITE, BLACK = (102, 119, 153), (255, 255, 255), (0, 0, 0)
PLAT, PLAT_DARK = (221, 221, 221), (153, 153, 153)
GLOBE_SRC = Image.open(ROOT / "docs/images/map.png").convert("RGB")


def globe_picture(w, h):
    scale = min(w, h) / 480
    img = GLOBE_SRC.resize((round(640 * scale), round(480 * scale)), Image.NEAREST)
    out = Image.new("RGB", (w, h), GLOBE_SRC.getpixel((0, 0)))
    out.paste(img, ((w - img.width) // 2, (h - img.height) // 2))
    return out


def menu_bar(d, width, window_menu=False, open_menu=False):
    d.rectangle([0, 0, width, 19], fill=WHITE)
    d.line([0, 19, width, 19], fill=BLACK)
    x = 12
    for name in ["File", "Edit", "View"] + (["Window"] if window_menu else []):
        if name == "Window" and open_menu:
            d.rectangle([x - 6, 0, x + text_width(name, 1.0, BOLD) + 6, 19], fill=BLACK)
            x = draw_text(d, x, 14, name, BOLD, 1.0, WHITE)
        else:
            x = draw_text(d, x, 14, name, BOLD, 1.0)
        x += 16
    return x


def check_mark(d, x, baseline):
    d.line([(x, baseline - 4), (x + 3, baseline), (x + 9, baseline - 9)], fill=BLACK, width=2)


def window_menu_dropdown(d, left, items):
    w = 200
    height = 6 + sum(8 if text == "-" else 16 for text, _, _ in items)
    d.rectangle([left, 20, left + w, 20 + height], fill=WHITE, outline=BLACK)
    y = 24
    for text, key, checked in items:
        if text == "-":
            d.line([left + 2, y + 4, left + w - 2, y + 4], fill=PLAT_DARK)
            y += 8
            continue
        if checked:
            check_mark(d, left + 6, y + 12)
        draw_text(d, left + 22, y + 12, text, FONT, 1.0)
        draw_text(d, left + w - 44, y + 12, key, FONT, 1.0)
        y += 16


def chrome(d, content, title, active=True, grow=False, close=True):
    l, t, r, b = content
    d.rectangle([l - 1, t - 20, r, b], outline=BLACK, fill=WHITE)
    d.rectangle([l, t - 19, r - 1, t - 1], fill=PLAT)
    if active:
        for y in range(t - 17, t - 3, 2):
            d.line([l + 22, y, r - 4, y], fill=PLAT_DARK)
        w = text_width(title, 1.0, BOLD) + 12
        mid = (l + r) // 2
        d.rectangle([mid - w // 2, t - 18, mid + w // 2, t - 3], fill=PLAT)
        draw_text(d, mid - w // 2 + 6, t - 6, title, BOLD, 1.0)
        if close:
            d.rectangle([l + 6, t - 16, l + 17, t - 5], fill=WHITE, outline=BLACK)
    else:
        w = text_width(title, 1.0, FONT)
        draw_text(d, (l + r) // 2 - w // 2, t - 6, title, FONT, 1.0, (120, 120, 120))
    d.line([l, t, r - 1, t], fill=BLACK)
    if grow:
        d.rectangle([r - 15, b - 15, r - 1, b - 1], fill=PLAT, outline=PLAT_DARK)


def scroll_bar(d, x, top, bottom, rows, visible, offset=0, active=True):
    d.rectangle([x, top, x + BAR - 1, bottom], fill=PLAT if active else WHITE, outline=BLACK)
    d.rectangle([x, top, x + BAR - 1, top + 15], outline=BLACK)
    d.rectangle([x, bottom - 15, x + BAR - 1, bottom], outline=BLACK)
    if rows > visible and active:
        span = bottom - top - 32
        thumb = max(12, span * visible // rows)
        y = top + 16 + (span - thumb) * offset // max(1, rows - visible)
        d.rectangle([x + 1, y, x + BAR - 2, y + thumb], fill=PLAT_DARK, outline=BLACK)


def card_view(d, left, top, width, height, card, factor=DEFAULT_FACTOR, scrollbar_bottom=None,
              active=True):
    """The card drawing of Sidebar::draw_card, in a pane `width` x `height` at (left, top)."""
    rows = wrap(card, width - MARGIN - BAR - 4, factor)
    visible = visible_rows(height)
    d.rectangle([left, top, left + width - 1, top + height - 1], fill=WHITE)
    draw_text(d, left + MARGIN, top + 16, card.title, BOLD, factor)
    for i, row in enumerate(rows[:visible]):
        draw_text(d, left + MARGIN, top + HEAD + LINE * (i + 1) - 2, row, FONT, factor)
    scroll_bar(d, left + width - BAR, top, top + (scrollbar_bottom or height) - 1, len(rows),
               visible, 0, active)
    return len(rows), visible


def looks_rows(d, left, top, width, current=0):
    for i, name in enumerate(["Map", "Relief"]):
        if i == current:
            check_mark(d, left + MARGIN, top + i * ROW + 15)
        draw_text(d, left + MARGIN + 16, top + i * ROW + 15, name, BOLD if i == current else FONT,
                  1.0)


def label(d, x, y, text, fill=(255, 255, 255)):
    d.rectangle([x - 3, y - 12, x + text_width(text, 1.0) + 3, y + 4], fill=(0, 0, 0))
    draw_text(d, x, y, text, FONT, 1.0, fill)


def geometry():
    with tempfile.TemporaryDirectory() as tmp:
        exe = pathlib.Path(tmp) / "layout_dump"
        subprocess.run(["c++", "-std=c++20", str(HERE / "layout_dump.cpp"), "-o", str(exe)],
                       check=True)
        lines = subprocess.run([str(exe)], capture_output=True, text=True, check=True).stdout
    g = {}
    for line in lines.splitlines():
        scene, key, *nums = line.split()
        g[(scene, key)] = tuple(int(n) for n in nums)
    return g


# ---- scenes -----------------------------------------------------------------------------------
def desktop(width, height, **menu):
    img = Image.new("RGB", (width, height), DESK)
    d = ImageDraw.Draw(img)
    menu_bar(d, width, **menu)
    return img, d


def scene_sidebar(g, scene, size, card):
    img, d = desktop(*size)
    l, t, r, b = g[(scene, "window")]
    chrome(d, (l, t, r, b), "Globe", grow=True)
    gw = r - l - SIDEBAR_W
    img.paste(globe_picture(gw, b - t), (l, t))
    d = ImageDraw.Draw(img)
    x = l + gw
    d.line([x, t, x, b - 1], fill=BLACK)
    looks_rows(d, x, t, SIDEBAR_W)
    d.line([x, t + ROW * LOOKS, r - 1, t + ROW * LOOKS], fill=BLACK)
    rows, vis = card_view(d, x + 1, t + ROW * LOOKS + 1, SIDEBAR_W - 1, b - t - ROW * LOOKS - 1,
                          card, scrollbar_bottom=b - t - ROW * LOOKS - 1 - 15)
    label(d, l + 8, t + 20, f"{gw} x {b - t} globe pane")
    return img, (gw, b - t, rows, vis)


def scene_workspace(g, scene, size, card, open_info=True, open_palette=True, info_front=False,
                    menu_open=False, note=None):
    img, d = desktop(*size, window_menu=True, open_menu=menu_open)
    gl = g[(scene, "globe")]
    chrome(d, gl, "Globe", active=not info_front, grow=True)
    img.paste(globe_picture(gl[2] - gl[0], gl[3] - gl[1]), (gl[0], gl[1]))
    d = ImageDraw.Draw(img)
    if open_palette:
        p = g[(scene, "palette")]
        chrome(d, p, "Looks", active=False, close=False)
        looks_rows(d, p[0], p[1], p[2] - p[0])
    stats = None
    if open_info:
        i = g[(scene, "info")]
        chrome(d, i, "Information", active=info_front)
        stats = card_view(d, i[0], i[1], i[2] - i[0], i[3] - i[1], card, active=info_front)
    if menu_open:
        window_menu_dropdown(d, 12 + 3 * 0 + 120, [
            ("Globe", "", True), ("Information", "⌘I", open_info),
            ("Looks", "⌘L", open_palette), ("-", "", False),
            ("Standard Positions", "", False)])
    label(d, gl[0] + 8, gl[1] + 20, f"{gl[2] - gl[0]} x {gl[3] - gl[1]} globe window")
    if note:
        label(d, gl[0] + 8, gl[1] + 40, note)
    return img, stats


def scene_recovery(g):
    """Six lost Information-sized windows on 640 x 480 (half scale): red = where it was, green =
    where recover() puts it. A window that is still grabbable is not moved (not shown)."""
    half = 0.5
    names = ["offright", "offbottom", "offleft", "undermenu", "sliver", "huge"]
    sheet = Image.new("RGB", (3 * 330 + 10, 2 * 270 + 10), (40, 40, 40))
    for k, n in enumerate(names):
        panel, d = desktop(640, 480)
        gl = g[("s640", "globe")]
        chrome(d, gl, "Globe", grow=True)
        panel.paste(globe_picture(gl[2] - gl[0], gl[3] - gl[1]), (gl[0], gl[1]))
        d = ImageDraw.Draw(panel)
        lost, back = g[("recover", f"lost-{n}")], g[("recover", f"back-{n}")]
        clipped = (max(lost[0], 0), max(lost[1], 22), min(lost[2], 639), min(lost[3], 479))
        if clipped[2] > clipped[0] and clipped[3] > clipped[1]:
            d.rectangle(clipped, outline=(230, 60, 40), width=3)
        chrome(d, (back[0], back[1] + 20, back[2], back[3]), "Information")
        d.rectangle([back[0] - 1, back[1] - 1, back[2], back[3] + 20], outline=(40, 200, 80), width=4)
        panel = panel.resize((320, 240), Image.LANCZOS)
        pd = ImageDraw.Draw(panel)
        on_screen = clipped[2] > clipped[0] and clipped[3] > clipped[1]
        pd.text((6, 24), n + ("" if on_screen else " (entirely off screen)"), font=BOLD,
                fill=(255, 255, 255))
        sheet.paste(panel, (10 + (k % 3) * 330, 10 + (k // 3) * 270 + 20))
        ImageDraw.Draw(sheet).text((10 + (k % 3) * 330, 10 + (k // 3) * 270 + 4),
                                   f"lost {lost}  ->  {back}", font=FONT, fill=(255, 255, 255))
    return sheet


def save(img, name):
    OUT.mkdir(parents=True, exist_ok=True)
    img.quantize(colors=96, method=Image.Quantize.MEDIANCUT).save(OUT / name, optimize=True)
    print("wrote", OUT / name)


def fit_table():
    """Markdown: wrapped rows per card at the sidebar and Information-window text widths."""
    panes = [("sidebar 220", SIDEBAR_W), ("Information 300", 300)]
    out = ["| Card | Font stand-in | Rows @ sidebar 220 | Rows @ window 300 |", "|---|---|---|---|"]
    for key in ["city", "ambiguous", "country", "today", "stress"]:
        for fname, f in FACTORS.items():
            out.append(f"| {key} | {fname} | " + " | ".join(
                str(rows_needed(CARDS[key], w, f)) for _, w in panes) + " |")
    heights = [200, 240, 280, 320]
    out += ["", "Rows visible without scrolling (HEAD 22 + 4 reserved, 14 px per row):", "",
            "| Pane | Rows visible |", "|---|---|"]
    out.append(f"| sidebar card at 640 x 480 (428 px window, 44 px looks) | {visible_rows(428 - 44)} |")
    out.append(f"| sidebar card at 1024 x 768 (716 px window) | {visible_rows(716 - 44)} |")
    out += [f"| Information window 300 x {h} | {visible_rows(h)} |" for h in heights]
    out += ["", "Smallest Information height that shows each card without scrolling "
            "(300 wide):", "", "| Card | " + " | ".join(FACTORS) + " |", "|---|---|---|---|"]
    for key in ["city", "ambiguous", "country", "today", "stress"]:
        cells = []
        for f in FACTORS.values():
            need = rows_needed(CARDS[key], 300, f)
            cells.append(f"{need} rows = {HEAD + 4 + LINE * need} px")
        out.append(f"| {key} | " + " | ".join(cells) + " |")
    return "\n".join(out)


def main():
    g = geometry()
    c = CARDS["city"]
    img, s = scene_sidebar(g, "s640sidebar", (640, 480), c)
    save(img, "workspace-148-a-sidebar-640.png")
    print("sidebar 640:", s)
    img, s = scene_sidebar(g, "s1024sidebar", (1024, 768), c)
    save(img, "workspace-148-a-sidebar-1024.png")
    img, _ = scene_workspace(g, "s640", (640, 480), c, open_info=False, open_palette=False,
                             menu_open=True, note="Information closed: Window menu reopens it")
    save(img, "workspace-148-b-compact-640.png")
    img, s = scene_workspace(g, "s640", (640, 480), c, open_palette=False, info_front=True,
                             note="Information opened over the globe")
    save(img, "workspace-148-b-overlap-640.png")
    print("overlap 640:", s)
    img, s = scene_workspace(g, "s640naive", (640, 480), c)
    save(img, "workspace-148-b-beside-640.png")
    img, s = scene_workspace(g, "s1024", (1024, 768), c)
    save(img, "workspace-148-b-workspace-1024.png")
    print("workspace 1024:", s)
    save(scene_recovery(g), "workspace-148-recovery-640.png")
    # Card-fit sheet: the same two cards in the sidebar width and in the 300 x 200 window.
    sheet = Image.new("RGB", (1130, 440), DESK)
    d = ImageDraw.Draw(sheet)
    x = 10
    for key in ["city", "ambiguous"]:
        for label_text, w, h in [("sidebar card, 220 wide", SIDEBAR_W, 384),
                                 ("Information 300 x 200", 300, 200)]:
            chrome(d, (x + 1, 40, x + w, 40 + h), label_text, close=False)
            card_view(d, x + 1, 40, w, h, CARDS[key])
            x += w + 20
    save(sheet, "workspace-148-cards.png")
    print(fit_table())


if __name__ == "__main__":
    main()
