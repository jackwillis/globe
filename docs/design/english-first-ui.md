# English-first interface for v0.1

Globe v0.1 ships with **English UI text only**. The current place names and
information cards are English-facing; switching just the menus to an ASCII-only
Spanish approximation misrepresented the product's language coverage. The
incomplete Spanish table and its live selector have been removed.

The lightweight string-catalog pattern remains: edit `l10n/en.txt`, run
`python3 tools/gen_strings.py`, and check committed generated `Str` keys and
English string data with `python3 tools/gen_strings.py --check`. UI components
call `tr(Str::Key)`; the build embeds strings with no runtime text-file reads.
Metric/imperial units remain an independent user preference.

**Future localization:** define Unicode and cross-platform font/encoding
contracts (#84, #61) covering Classic Mac OS 9 and X11 before translating the
UI. Then extend catalog generation and locale handling. Preserve source
spellings, geographic IDs, aliases and provenance in the atlas pipeline;
never overwrite source names with ASCII fallbacks as part of UI simplification.
Installed-font support and renderable display fallbacks are separate concerns.

No fonts, atlas datasets, renderer changes or new dependencies are introduced.
