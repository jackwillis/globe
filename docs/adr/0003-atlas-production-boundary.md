# ADR 0003 — Separate Globe runtime from atlas production

**Status:** Architecture boundary accepted in principle (2026-11-07); source policy, internal formats, edition composition and schedule are **proposals pending experiments**.

## Context

Globe currently couples bundled data choices to runtime feature implementations: 110m Natural Earth coastline/borders and 177 country polygons, a 304-city Wikidata pack selected by >=1 million population and 30/country, ETOPO-derived relief, RESOLVE-derived land classes, and optional 0.5-degree relief. These are reproducibly pinned, strictly decoded and effective for a prototype, but the data selection omits small states/capitals, underrepresents geographic roles and has no rivers layer. Country polygon indices aren't durable identities. A decoded, valid pack is not proof of correct geography.

User direction: the repository should support **two coherent projects**: the interactive Globe application and a reproducible, transparent system of geographic data/rules that generates independent atlas editions. The user wants to direct data-side work in parallel with UI/runtime improvements.

## Accepted direction

1. **Two projects, one repository for now.**
   - **Globe Runtime:** platform-neutral interactive core, projection and render engine, user interactions, bounded loading, cache, and the X11/Classic Mac frontends; retain the Elm-inspired `Model / Msg / update / Effects` and the portable C++20/Retro68 constraints.
   - **Globe Atlas Builder (working name):** source acquisition and pinning, source licensing and provenance, canonical feature identities, country/city/river selection, names and geopolitical conventions, transformations/generalization, QA, pack generation, edition definitions and reproducibility; may use modern Python and GIS dependencies **only on build hosts**.
   - Interface: versioned, strict and bounded **atlas pack contracts**, a compatibility/edition manifest, and conformance tests. No runtime dependency on Python, GIS libraries, network services, or build-time source formats.

2. **Geographic existence, geometry and presentation are distinct.** A microstate should exist in the catalog even when the chosen coarse polygon omits it. A place's inclusion differs from showing its symbol or label at a given zoom. Explicit entity IDs should be stable across sources and detail levels; pack-local geometry indexes must not act as canonical IDs.

3. **Correctness is multi-dimensional:** source-integrity, reproducibility, entity coverage, identity joins, geometry/topology, attributes with time/definition, cultural/geopolitical interpretation, scale-dependent visibility, picking, and documented omissions. Source hashes and CRCs prove byte integrity, **not** these semantic claims.

4. **Offline-first, edition-driven.** The same renderer can consume a compact base atlas, a standard atlas, and optional expanded geographic layers. Source data and selection rules should be versioned independently of engine releases; distribute only validated, license-reviewed output. A small fallback atlas remains usable without network access.

5. **Investigate before committing to formats.** No premature universal GIS object model, embedded SQL dependency, tile-server service, or unmeasured new decoder. Separate typed geometry/point/raster layers with a small shared container/manifest is a favored *hypothesis*. Prototype with real 50m/10m data, benchmark G4 memory and display cost, then decide.

## Responsibilities and proposed workflow

| Concern | Atlas Builder | Runtime |
| --- | --- | --- |
| source download, license, snapshot hashes | owns | never fetches sources as a build prerequisite |
| canonical entities, aliases, capital/country relations | curates and validates | uses stable compact identifiers |
| worldviews, disputed borders, classifications | explicit build policies and provenance | exposes truthful metadata / optional selection later |
| vector geometry, generalization, topology | builds and validates | clips/projects/draws/picks |
| ranking city/river importance and label priorities | produces static metadata | resolves visible screen-space candidates and collisions |
| elevation, masks, biomes | converts and documents semantics | uses typed grids and styles |
| compression, pack versions, manifest | emits and tests | verifies bounds/CRC/version/dependency before activation |
| runtime resource lifetime / budget | reports estimated resident/peak memory | enforces load/evict and safe publication |
| hardware and interactive tests | provides expected landmark/coverage fixtures | verifies performance and interaction on target |

## Preliminary source hypotheses — not approvals

- Natural Earth 110m/50m/10m as aligned global cartography; **country/tiny-country points**, countries/map units and subunits as different concepts.
- Natural Earth populated places as the initial cartographic roster; Wikidata QIDs/attributes and editorial corrections as audited supplements. Stop using maximum historical population plus one-million minimum as inclusion policy.
- Natural Earth ranked river/lake centerlines and lakes as first hydrography layer. Keep lake connections and intermittent streams distinguished from ordinary rivers; check reach/shoreline coherence.
- Retain NOAA ETOPO and RESOLVE land classes with existing provenance; explicitly separate physical elevation, water/land masks, ice and biome semantics.
- GeoNames, Who's On First, OpenStreetMap, HydroRIVERS or other detailed providers are **candidates only**, subject to attribution, license, compatibility and data-quality review.

## Questions to resolve with data before building the new formats

1. What is Globe's default political entity inventory: sovereign states, countries, territories, map units and disputed areas? How do we expose the worldview without asserting that all sources agree?
2. What is the smallest complete-enough global base catalog (including microstates, capitals and representative points), and what are deliberately absent details?
3. How are feature identities stable and how do crosswalks deal with names, parent-child relations, boundary changes and temporal attributes?
4. Which city selection rules produce a geographic atlas rather than a population leaderboard? How should cities be included, represented and labeled independently?
5. Which river geometries/types/ranks align with lakes and coastlines at each scale? What source limitations remain?
6. Should data be bundled as one file, a manifest plus typed packs, or a hybrid? Compare file size, decoded residency, peak allocations, load time, code size and safe fallbacks on G4/Classic.
7. Which edition compositions and resource budgets are justified by *measured* G4/OS9 results? How do we maintain compatibility across updated data releases?

## Proposed repository boundary (illustrative only)

```text
src/ include/ os9/               # application/runtime
tools/atlas-builder/             # existing tooling to consolidate, not move blindly
tools/places-builder/
tools/country-builder/
atlas/                           # proposed production project
  README.md
  sources.lock.json              # source URLs, exact versions, hashes, licenses
  catalog/                       # reviewed exceptions, IDs and relations
  rules/                         # selection, labeling, geopolitics
  editions/                      # base, standard, extended build recipes
  qa/                            # omissions, semantic checks, visual fixtures
data/                             # generated small packs + source/provenance documentation
docs/                             # contracts, ADRs, target benchmarks
```

Refactor the current tools into `atlas/` **only after** inventorying scripts, tests and build integration. The initial data audit should make no production-format or renderer change.

## Immediate data-side work (the first independently reviewable milestone)

- Inventory existing 177-country/304-city selection and known exceptions, with a reproducible manifest of source records and identities.
- Obtain/pin and compare suitable Natural Earth 50m/10m countries, tiny-country points, populated places, rivers and lakes; record source versions, terms, hashes and transformation environment.
- Produce coverage and conflict reports: mandatory microstates/capitals, questionable city inclusion, country/territory classification, river topology and naming; mark unverified claims.
- Run at least three candidate edition compositions through a *build-time only* prototype: core/standard/extended; report disk size, point counts, decoded/peak-memory estimates, and G4 risk.
- Write a decision memo proposing canonical IDs, rules, source policy, format migration and code integration; do not commit to a new on-device decoder or edit current packs without review.

## Interfaces with current backlog

- Parent areas: [#5 Offline data and acquisition](https://github.com/jackwillis/globe/issues/5), [#4 Geographic discovery](https://github.com/jackwillis/globe/issues/4), and [#35 Usability](https://github.com/jackwillis/globe/issues/35).
- Related existing work: [#24 place search](https://github.com/jackwillis/globe/issues/24), [#26 resource ownership](https://github.com/jackwillis/globe/issues/26), [#29 conversion](https://github.com/jackwillis/globe/issues/29), [#30 geographic LOD](https://github.com/jackwillis/globe/issues/30), [#31 pack format](https://github.com/jackwillis/globe/issues/31), [#36–#39 text/labels](https://github.com/jackwillis/globe/issues/36), [#48 external files](https://github.com/jackwillis/globe/issues/48), [#50 full edition data](https://github.com/jackwillis/globe/issues/50).
- **Issue tracker is authoritative.** Do not duplicate existing runtime work or turn preliminary data concepts into release blockers without owner review.

## Out of scope of this decision record

No immediate changes to the engine, new source downloads, generated production packs, source license amendments, automated G4 activity, release publication or migration to a separate repository. This records a direction and research gates, not an authorized implementation sprint.
