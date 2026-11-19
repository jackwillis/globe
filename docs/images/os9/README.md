# Authentic Mac OS 9 screenshots

Captured for [#118](https://github.com/jackwillis/globe/issues/118) on 2026-11-17 on a Power Mac G4
running **bare Mac OS 9.2.2** (1280×960 display), with Command-Shift-3. These are real application
captures, not headless renders and not Tiger Classic.

- **Build:** the owner's `Test 116` copy, commit `110a197638fedb267c538f36b1fb25747b828734`
  (same application code as `main` at `ff98bd7`). The About window reads `Version 0.1.0`, which does
  not distinguish it from the v0.1.0 release, so the build identity is the owner's statement.
- **Conversion:** the PICT originals (v2, 32-bit DirectBits, packType 4) were decoded losslessly to RGB
  PNG without scaling or cropping and saved with Pillow `optimize=True`. The PICT originals are not
  committed. Their SHA-256 sums are in the table. The decoder is a small script kept outside the repository.
- **Website:** `site/images/` holds cropped copies (see `docs/website-maintenance.md`).

| File | Subject | Source PICT SHA-256 |
| --- | --- | --- |
| `map-window.png` | Map look, window on the desktop (also the website hero and Map crops) | `90a3ac369c29884aee01ce5bbe147b37d0b1b6b34862855326c8b27166dd91f2` |
| `relief-window.png` | Relief look (website Relief crop) | `14008f7174fc2cac91be717f9b97626c2579f2b81268895290460e4681291c63` |
| `country-selected.png` | Laos selected, Relief look (website selection figure) | `93e3b3ed9fe1d715820dfae00edb037aa1eb685e6dbb6f055ff411f1b40abd33` |
| `preferences-measure.png` | Preferences window over a measured distance | `6cb746ed54a37027026d28e9b1df734ebdf579af3acbf08d021bf6715e60a299` |
| `controls-night-side.png` | Controls help list with the night side on | `03f5e3e3dbe438858d50641a00e35b6f8b10ce8f9f3c295ce89caa7b36695634` |
| `about.png` | About window, version 0.1.0 | `d8fbbd402a459646b4c3f89e72686f474d6f8d98e44e7cb0dcc6d8beac147a03` |
| `memory-alert.png` | The #116 alert at a 12 MB partition (build `110a197`) | `6cd82f95bbc666386e3b51cdadf4949dbab3cff2aa039de73adc5f32780725e2` |

The originals stay on the owner's file share and laptop; the sums above identify them.

Crops used on the site (x, y, right, bottom in the full-screen image): hero `map-window.png`
(95, 28, 1107, 940); Map and Relief (150, 150, 830, 830) from `map-window.png` and `relief-window.png`;
selection `country-selected.png` (115, 40, 1087, 918).
