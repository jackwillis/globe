# Globe — Product Direction and Architecture Persistence Brief

**Status:** Owner-directed planning brief
**Date:** November 10, 2026
**Scope:** Future product direction, atlas data architecture, geographic navigation, knowledge, cartographic typography and release planning.

## 1. Purpose

Persist the product and architecture direction developed after the first successful performance-optimized bare Mac OS 9 testing session.

Do not treat this as authorization to implement every proposed feature. This brief establishes product intent, records agreed architectural principles, and identifies technical questions requiring experimentation.

The immediate objective is to organize the existing repository's plans coherently, without expanding the v0.1 release gate.

## 2. Product vision

Globe is an offline-first, free-software geographic hypermedia atlas for classic computers, particularly Mac OS 9 on PowerPC.

It combines a navigable, software-rendered Earth with geographic names, administrative regions, facts, articles, relationships, saved places, and eventually more naturalistic rendering, speech and astronomy.

The application should feel like a native classic Macintosh atlas, drawing inspiration from traditional cartography, Encarta and HyperCard without reproducing proprietary software.

Its defining experience is spatial and semantic exploration:

- Discover a place visually.
- Select its label or geographic feature.
- Read facts and linked content about it.
- Follow relationships to other places.
- Navigate the globe accordingly.
- Return through navigation history.
- Save meaningful places for future visits.

Globe should remain useful without a network connection or external data packs.

## 3. Established product priorities

### Immediate priority: useful geographic navigation

The next major development focus is:

1. Stable geographic entity identity.
2. States, provinces and administrative regions.
3. Beautiful, readable, interactive map labels.
4. Entity-based selection and navigation.
5. Linked information cards and Back/Forward behavior.

These should form one coherent vertical slice rather than unrelated feature implementations.

A representative acceptance journey is:

**United States → Wisconsin → Madison → Back to Wisconsin**

Use additional fixtures involving microstates, non-US administrative hierarchies, ambiguous settlement names and geographic edge cases.

### Knowledge experience

Globe should eventually support **both structured facts and readable articles**.

Facts require meaningful qualifiers, source attribution and dates where applicable. Articles should support headings, paragraphs, references and links to other geographic entities.

The initial implementation should use small, curated examples rather than ingesting entire encyclopedias.

### Long-term experience goals

The owner also wants:

- Offline location search and saved favorite places.
- Higher-resolution geographic data.
- Rivers, lakes, mountains and richer region inspection.
- Compass rose, scale bar and stronger cartographic labeling.
- Elevation that looks geographically convincing.
- Naturalistic, satellite-inspired land-cover rendering, including visually differentiated urban areas.
- Spoken place names, using native Macintosh speech facilities where appropriate.
- Stars, Sun, Moon and planetary context.
- Geographic hypermedia capable of supporting increasingly rich relationships and content.

These are product goals, not commitments for the next release.

## 4. Architecture principles

### Geographic identity

Globe owns canonical, stable entity identifiers.

External systems such as Wikidata, Natural Earth and ISO classifications are connected through explicit concordances. No external identifier or polygon index should automatically become the universal identity.

Entity identity, geometry, labels, source assertions, knowledge and presentation are distinct concepts.

Names, coordinates and proximity alone are insufficient to establish identity.

The builder must preserve unresolved or disputed matches for review.

### Runtime boundaries

Retain the existing portable C++20 and Elm-inspired Model / Msg / update / Effects architecture.

User intent—selecting, navigating, searching, following links and saving favorites—should be expressed independently of platform-specific UI actions.

Keep the mutable application Model small. Large immutable catalog data, geometry, search indexes and knowledge resources belong in separately managed, bounded resources.

Do not introduce an embedded browser, scripting environment, general-purpose graph database, or runtime GIS dependency as a prerequisite for geographic hypermedia.

### Resource references

Stable geographic references must remain meaningful across atlas editions, geometry-detail changes and optional pack loading.

Do not keep raw pointers into evictable data as durable navigation or favorite references.

The exact runtime ID representation, reference encoding and pack layout require prototype validation.

### Data and provenance

Maintain two cooperating development areas in one repository:

- Globe's portable application/runtime.
- The reproducible atlas production pipeline.

Atlas production owns source acquisition, provenance, geographic identity, editorial rules, data validation, selection policies, generated indexes and edition recipes.

Runtime owns validated pack loading, rendering, navigation, resource lifetimes and memory budgeting.

Prefer versioned, typed, bounded binary resources compiled on modern machines. Do not require Python or GIS libraries on Mac OS 9.

Source assertions, curated claims and runtime displays should remain distinguishable.

Evaluate Natural Earth, Wikidata, Wikipedia and suitable elevation/land-cover sources individually for licensing, quality, coverage and update behavior.

Existing GPOL/GPLC formats should remain supported during incremental migration.

## 5. Cartographic typography direction

Typography is a core part of Globe's product identity, not a decorative afterthought.

Preserve Helvetica for functional UI roles and Palatino for geographic/editorial roles, with appropriate installed-font fallbacks.

The owner is interested in geography-aware typography: labels that avoid collisions, fit available space, relate to geographic shapes, and eventually follow rivers or other linear features.

**Preferred first approach:** retain native font rendering through Classic QuickDraw and platform-appropriate X11 facilities, while building a portable label-placement engine.

Separate:

- Text measurement.
- Geographic anchor generation.
- Label candidate ranking.
- Collision detection and placement.
- Interactive hit testing.
- Platform-specific glyph drawing.

The same accepted placement records should drive visual presentation and label selection.

Use generous label-level interactive targets rather than requiring users to click individual glyph pixels.

### Glyph-level experiments

Investigate whether native font metrics and conservative per-glyph ink bounds allow denser, more expressive labels without introducing a custom font rasterizer.

Compare:

1. Whole-label bounding rectangles.
2. Padded per-glyph rectangles.
3. Coarse occupancy masks generated through native text rendering.

Measure visual quality, readability, collision rate, CPU cost and memory on the actual G4.

Advance widths are not necessarily exact ink bounds. Kerning, italic overhangs, text encodings and font availability must be accounted for.

Native QuickDraw can position individual characters, but true rotated glyph rendering requires additional transformation work. A possible future experiment is to rasterize small glyph images natively and transform those bitmaps.

Do not adopt libschrift, FreeType or a bundled glyph atlas without measured evidence and a reviewed font-distribution policy.

Curved text, river-aligned text and glyph-level collision remain experiments—not release prerequisites.

### Placement behavior

Labels should be geographically meaningful and visually stable.

Consider alternate anchors, collision priorities, viewport/horizon constraints, text hierarchy, and hysteresis to avoid jitter during rotation.

Preserve interaction responsiveness, progressive rendering, deterministic placement for given font metrics, and the authoritative framebuffer's existing correctness tests.

## 6. UX decisions and hypotheses

Selection and navigation are separate operations.

Selecting a geographic label should show its entity information without necessarily moving the camera.

Explicit navigation should center or zoom toward a target and record meaningful navigation history.

Back/Forward should restore the relevant entity and camera state.

Cities remain individually selectable point features. Country and administrative region labels are independently interactive, while polygon picking can remain a fallback.

Prefer the existing native single-window interface with contextual information in the sidebar. Search, longer articles and optional developer facilities may use additional native windows when justified.

Avoid permanent controls that substantially reduce the globe's visible area at 640×480.

## 7. Release direction

Preserve the existing milestone structure.

**v0.1 — The Classic Atlas**

Finish native hardware acceptance, minimum-memory validation, release packaging and essential polish. No new hypermedia features should block this release.

**v0.2 — The Atlas on the Map**

Prioritize stable identity foundations, administrative regions, useful labels, clickable names, navigation and basic linked information. A small, complete geographic navigation example is preferable to broad but incomplete feature coverage.

**v0.3 — Memory and Packs**

Develop safe external-resource loading, memory budgets, persistence foundations and the ability to refer to entities independently of optional loaded resources.

**v0.4 — The Expandable Atlas**

Support richer regional editions, broader catalogs, refined geography, additional knowledge content and higher-detail rendering.

Search, favorites, articles and knowledge integration may be prototyped earlier, but comprehensive versions should not become automatic blockers for earlier milestones.

Future themes may include a richer geographic knowledge system and Earth-and-sky visualization. Do not create new version commitments without reviewing existing milestone scope.

## 8. Project-management instructions

Review existing issues and ADRs before creating new ones.

Use existing #52/#53 for atlas production and geographic identity; #35/#36/#38/#61 for map typography and usability; #24 for search; #54 for memory management; and #48/#49 for external packs and read-only-volume behavior.

Propose a small cross-cutting tracking issue for the geographic identity and navigation vertical slice.

Create narrowly scoped implementation issues only after their prerequisites and acceptance criteria are understood.

Mark research, owner decisions and implemented behavior distinctly.

Do not duplicate source-of-truth content between GitHub Issues, ADRs and the roadmap.

For any new issue, include the intended user experience, dependency links, explicit scope boundaries, acceptance criteria, required tests and hardware evidence.

## 9. Agent responsibilities

**Codex:** Geographic identity, concordances, data pipeline, binary format experiments, indexing, portable placement algorithms, testing and performance.

**Claude Code:** Macintosh UI, native typography adapters, interaction design, navigation behavior, information cards, keyboard support and user-facing documentation.

Both agents should review shared interfaces and independently review the other's changes where appropriate.

Initial work should be research and small prototypes, not a large simultaneous rewrite.

## 10. Immediate next steps

1. Persist this vision and reconcile it with the existing roadmap.
2. Draft an ADR for entity identity, concordances and navigation reference semantics.
3. Define the minimum shared entity/navigation interfaces.
4. Prototype a tiny real-world administrative catalog and synthetic navigation model independently.
5. Prototype native label measurement and collision placement.
6. Validate the integrated Wisconsin → Madison experience on Classic Mac OS.
7. Use findings to refine the v0.2 issue structure.

## 11. Explicit non-goals

This planning phase does not authorize:

- Replacing existing production packs.
- Merging experimental city-selection policy without review.
- Introducing a custom font renderer without comparison.
- Ingesting all of Wikipedia.
- Adding an embedded HTML browser.
- Implementing photorealistic terrain or astronomy now.
- Turning all proposed features into v0.1 or v0.2 release blockers.
- Publishing a release or changing protected hardware configuration.

**Guiding principle:** Make Globe a more useful, expressive, explorable atlas while preserving its responsiveness, offline independence, Macintosh character and reproducible geographic integrity.
