# Poodle Earth 0.1 — public preview (release notes draft)

A free, offline, software-rendered interactive Earth for Mac OS 9 on PowerPC Macs. Drag to turn
the globe, zoom, click a country or city for its information card, switch between a biome-tinted
Map look and a Relief look, shade the night side, and measure a great-circle distance. Nothing is
downloaded and nothing is written to disk.

**Download:** `PoodleEarth-0.1.0-<rev>.sit` (StuffIt) holds the application, ReadMe, the License
(two files) and DataCredit. `PoodleEarth-0.1.0-<rev>-source.tar.gz` is the complete corresponding source of exactly
these binaries. `RELEASE.json` and `SHA256SUMS` record sizes, checksums, the source revision and the
toolchain.

**Tested:** Power Mac G4 AGP 467 MHz, 256 MB, bare Mac OS 9.2.2 — see the acceptance record in
issue #95. Other Macs and systems are untested. Application memory: Preferred 24 MB, Minimum 12 MB.

**Known limits:** the globe edge is occasionally left unfinished after some sequences (#74); small
data set (1:110m, one-degree terrain, 304 cities); ASCII names; no custom icon; window size is
limited by available memory (#103).

**Licence:** GPL-3.0-or-later. Data: Natural Earth (public domain), NOAA ETOPO 2022 (CC0), Wikidata
(CC0), RESOLVE Ecoregions 2017 (CC BY 4.0, adapted). Full notices in the DataCredit file and the
About window.
