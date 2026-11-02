# Atlas data provenance

`atlas.gatl` is generated; the format is described in `docs/atlas-format.md`. The same text is
embedded in the file itself.

| Layer | Dataset | Version | Source ZIP SHA-256 |
| --- | --- | --- | --- |
| Coastline | Natural Earth 1:110m coastline | 4.1.0 | `664449b39070027e882abb295974d182afec18ca21107273d17e9e8bf6f64817` |
| Borders | Natural Earth 1:110m admin-0 boundary lines (land) | 5.1.0 | `bb2954981570498a4b4344f8e83fa54caa5858e15e800094112407d2bd2f940d` |

- Coastline ZIP: https://naturalearth.s3.amazonaws.com/110m_physical/ne_110m_coastline.zip
- Borders ZIP: https://naturalearth.s3.amazonaws.com/110m_cultural/ne_110m_admin_0_boundary_lines_land.zip
- Terms (public domain): https://www.naturalearthdata.com/about/terms-of-use/
- Retrieved 2026-11-01. The hashes match those recorded for the v1 prototype, retrieved the same
  day. The ZIPs are not committed (about 140 KB); the builder verifies them.
- Natural Earth draws de facto boundaries. The border layer is a cartographic reference, not a
  statement about sovereignty.

Regenerate (needs only Python 3's standard library):

```
python3 tools/atlas-builder/build_atlas.py ne_110m_coastline.zip \
    ne_110m_admin_0_boundary_lines_land.zip data/atlas.gatl
```

The output is byte-identical for the same inputs (SHA-256 of `atlas.gatl` as built on
2026-11-01: `ec2f5bde911fc131610786520a36482a98ad7312f29b953abc7ba6ccda8df773`).
