# Accepted checkpoint continuation — catalog and memory primitive

Jack accepted the prior research checkpoint and selected **both follow-ups, starting with the atlas
catalog**, within a two-hour unattended limit. This continuation implements those two slices. It
changes no production packs, recipes, rendered pixels, fonts or source locks. A separately tested
optional framebuffer accounting hook is the first allocation-site slice; frontend wiring and guards
remain unchanged.
No individual geographic assertion is declared approved merely because the engineering checkpoint
was accepted. #53's scope/closure and production adoption remain separate from this code delivery.

## New local commits and recoverable workspace

- `7a6652f`: expanded source catalog, stable allocations, evidence-bound reviews and additive role selection.
- `c419f5c`: independent fixed-storage memory ledger and move-only accounting ownership tests.
- `1eeb070`: optional framebuffer pixel/coverage accounting, failure-safe ownership tests and ABI probe.
- `dfb3b3c`: normal-build and sanitizer test registration after confirming no pending CMake edits elsewhere.
- A final documentation commit contains this note, verification and the atlas entry-point update.

Worktree `<temporary-test-binary>`, branch `codex/atlas-contract`; original runtime base `52ccb33`.
The prior commits `2afaebe` and `3e2057a` remain unchanged ancestors. No local merge, rebase, push,
release, target-machine access or modification of another worktree was performed. Local commits
live in the owner's main Git repository even if the temporary worktree directory disappears.
The final documentation commit leaves this worktree clean; inspect `git status` before continuing.

Other sessions continued concurrently. At the final inspection, local main was `0c2087a`, with
`bf33b42` (menu smoke-test observation), `e06a0c6` (packaging helper), and `0c2087a` (StuffIt CRC
updates) newer than cached `origin/main` at `e3de54a`. These change packaging/UX documentation/tools
and no atlas/data files.
`<local-checkout>` was on `claude/menu-bar` at `bf33b42`.
The original renderer WIP remains unstaged in `<local-checkout>`:
`include/globe/render.hpp` and `src/render/render.cpp`. A packaging-source edit observed during
this continuation was subsequently committed by its owner as `0c2087a`. None of that work was
touched; the old stash is untouched. The progressive
resolution worktree remains on `codex/os9-cooperative-scheduling` at `f8b8159`.
No concurrent work was adopted into this branch; Jack owns final integration.

## Expanded catalog and important new evidence

The persisted registry now has **808 proposed source concepts**, preserving all 35 original entity
IDs and original fixture assertion IDs. This is 258 country records, 298 map-unit records and 252
settlements. It is not a count of sovereign states or a deduplicated world inventory. Every layer
keeps its source scope even when NE_ID/QID/ISO values coincide. New allocation is explicit; normal
generation cannot silently mint IDs, rebind ordinals, change source revisions or drop old IDs.

The generated catalog has 1,059 assertions, 21,818 exact/localized/fallback names and 808 points.
All 23,685 reviewable rows remain unreviewed. All extracted fields and source representative
coordinates verify against the three pinned archives. Eight prior planar outside-label diagnostics
are referenced with their report hash; the points remain unverified for spherical picking.

The capital-role roster is broader than the earlier flag-only check:

- 214 records have a national or alternate flag; 213 also have an Admin-0 source class.
- 30 further records have only an Admin-0 class, giving **244 source-role candidates**.
- Juba has the source class `Admin-0 capital` but both flags zero. Its place code SSD needs a
  candidate alias to country code SDS. This is retained rather than repaired.
- Yangon/Johannesburg show why class-only inclusion cannot certify current-capital status.
- Quezon City is flag-only. Regional/territory roles stay distinct from national roles.
- Longyearbyen targets a map-unit representation; no legal sovereignty conclusion follows.

The original recipes omit 11/9 of this broader roster in Compact/Standard, including Juba in both.
An **additive experiment** retains all original selections and includes the entire source-role
review roster: Compact 373 → **384**, Standard 1,362 → **1,371**. Compared with a baseline carrying
the same explicit role-review metadata, actual gzip payload increases **3,234 / 3,076 bytes**;
JSONL increases 42,007 / 35,399 bytes. No record is promoted to a current national capital, no
capital flag changes, and no shipping/experimental recipe is changed. Visibility/text placement
remains separate. See `role-selection.json` and `catalog-inventory.json` for complete source cases.

`review-decisions.json` records research acceptance separately from an empty geographic decision
list. The new overlay changes only named/dated review fields. Its SHA-256 basis binds claim meaning,
source records/digests, points and fallback lineage. A changed relation or source evidence fails as
stale; duplicate/missing targets fail; unresolved candidates cannot become accepted equality via a
status toggle. A 1,869-entry queue covers assertions, points and lossy fallbacks without flooding
review with every exact localized name. No human geographic decisions were fabricated.

Full 11,885,212-byte catalog outputs remain `<temporary-test-binary>{a,b}.json`; the review queue is
`<temporary-test-binary>`. Full role-selected JSONL/gzip outputs are in
`<temporary-test-binary>{a,b}`. They are reproducible local evidence outside Git, not releases.
Committed source/allocation/review files and summaries are enough to recover and regenerate them.

## Independently scoped memory work

`include/globe/memory_ledger.hpp` implements fixed-storage category counters and simultaneous total
peaks, checked payload arithmetic, allocation-failure events, snapshots, peak reset and move-only
`MemoryCharge` accounting tickets. Ticket movement transfers an existing charge without reacquiring;
assignment releases destination accounting first. Tickets never allocate or free payload themselves;
the ledger must remain stable and outlive them. Errors are sticky and do not wrap numeric counters.

The independent project under `atlas/research/ledger/` provides two reproducible CTest targets.
After inspecting main and Claude's worktree for pending build edits, one root `add_subdirectory`
hunk now registers both for normal CI, separately from the committed menu-test hunk. They also
inherit root `GLOBE_SANITIZE`. No other worktree or platform build file was edited.
`Framebuffer::create` accepts an optional borrowed ledger/category, counts pixels/coverage as two
successful allocations and records actual allocation failures. Default two-argument callers remain
uninstrumented. No frontend supplies a ledger yet, and the 32 MiB guard remains unchanged. This is
not full completion of #55/#59, budget/degradation (#56), eviction (#57), or a CI gate (#60).

The framebuffer test independently observes exact allocation bytes, injects failure at each
allocation, preserves the old usable buffer during replacement failure, verifies same-/cross-ledger
moves and cleanup, compares pixels, and observes no allocation during 10,000 draw/measurement
iterations. Accounting failure flags diagnostics without changing resource-allocation success.
The factory's default behavior and legacy failure paths remain tested. Only framebuffer header/
implementation changed; the owner's `render.hpp`/`render.cpp` WIP is untouched.

Tests cover overlapping/disjoint peaks, failure cleanup, reset while live, zero-byte events,
move/self-move/cross-ledger ownership, invalid categories, underflow and byte overflow. A separate
reference model checks 10,000 interleaved events. Independent global new/new[] hooks observe zero
allocation calls throughout the tested operations. On Linux x86-64, ledger/ticket inline sizes are
248/24 bytes. Framebuffer inline size grows 40 → 88 bytes on that ABI. Compile-only assembly
constants show PowerPC ledger/ticket 244/16 bytes and framebuffer 28 → 60; m68k ledger/ticket
242/14 and framebuffer 26 → 54. These are compiler ABI/object-storage facts, not target
heap/partition observations. Exhausting uint64 event counters is not part of the bounded tests; those overflow checks are present in code.

## Tests, reproducibility and limits

- **79 Python tests passed**: audit 8, concordance 9, catalog 24, selection 9, inventory 13,
  review overlay 12, additive role selection 4. Each is executable as `python3 atlas/research/test_*.py`;
  the optional GIS geometry suite remains separate and was not rerun.
- Both standalone accounting CTests passed with GCC 16.2.1 and Clang 22.1.8, no exceptions/no RTTI.
  The framebuffer target suppresses preexisting signedness warnings; no unrelated drawing cleanup.
- Both ASan/UBSan accounting CTests passed. LeakSanitizer fails under the sandbox's ptrace, so the verified run
  used `ASAN_OPTIONS=detect_leaks=0`; leak checking is explicitly not claimed.
- Final ledger/framebuffer test units and framebuffer implementation compile with Retro68 GCC 16.1.0
  PowerPC and m68k in an offline cached container. Compile-only, no target execution, no 68k runtime-support claim.
- Two complete inventory catalogs/summaries match byte-for-byte; two additive role reports and all
  eight full JSONL/gzip outputs match. Pinned archive/point verification and 58 existing semantic
  cross-report checks pass. Original fixture/source/recipe data remain unchanged.
- Python compile checks, clang-format 22.1.8, policy and whitespace checks pass.
- **52 root CTests pass across the relevant phases:** the existing 50 passed after framebuffer
  instrumentation (40 sandbox + 10 private-display retry), then both newly registered tests passed
  in the root build. Pixel pins/packs and platform adapters remain unchanged. The optional
  accounting API is not yet used by the frontends.
- Root sanitizer configuration also passes both accounting tests. Configuration-only checks
  register 0 tests with `GLOBE_BUILD_TESTS=OFF` and 42 with X11 discovery disabled; those alternate
  full suites were not executed.

Commands, hashes, measured host builder RSS/time, compiler metadata and limitations are in
`continuation-verification.json`; entry-point commands are in `atlas/README.md` and
`atlas/research/ledger/README.md`. Process RSS is build-host memory. Payload sizes and spherical
distances are measurements; hypothetical record residency remains an estimate. No G4 memory,
latency, installed glyph, current-capital or geopolitical completeness is established here.

A final offline **complete Classic PowerPC cross-build/link** passed for baseline `52ccb33` and
current `1eeb070`, using private `git archive` copies and the cached Retro68 image. Linked text
section grows 499,712 → 503,808 bytes (**+4,096**); linked data (33,328) and BSS (35,040) are unchanged.
PEF/data-fork size grows 541,674 → 545,770 bytes; MacBinary grows 542,336 → 546,432 bytes.
The framework automatically generated its standard wrapper/disk-image files in those temporary
build folders; no release packager was invoked and nothing was installed, committed or published.
This is source/link/size evidence, not a hardware run or release acceptance. Framebuffer ticket
inline costs can also enlarge enclosing objects/allocations; total app heap remains unmeasured.

## Specific next work and coordination

Atlas: process the review queue, beginning with source-role disagreements and exceptional crosswalks;
add independent, dated evidence rather than accepting all class/flag values as current geography.
A source-version reconciliation contract should distinguish stable records, changed claims and
retired concepts before new source snapshots can be adopted. This is the next small data issue.
No source-policy editorial decision or current-capital override is made in this continuation.

Memory: coordinate stable frontend ledger ownership, then instrument a
scene constructor with tickets and failure injection. Framebuffer accounting is already opt-in.
Reconcile counted heap payload against logical `bytes()` methods and inline storage before connecting budgets/diagnostics or replacing guards.
Claude owns platform Memory Manager/partition observations and frontend reporting.

Claude's font/label handoff remains: atlas supplies stable IDs, attributed role candidates,
languages, exact names and explicit lossy fallbacks; the installed-font adapter measures the actual
chosen string and reports missing glyphs. No font files, glyph packs or renderer implementation
are added. Relevant boundaries are `catalog/README.md`, `reviews.py`, `memory_ledger.hpp` and the
standalone ledger README. These requests are durable local notes, not messages or remote issue edits.
