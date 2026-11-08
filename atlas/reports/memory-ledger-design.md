# Proposed first memory-ledger slice (#55/#59/#17)

**Continuation:** Jack accepted the research checkpoint and authorized both follow-ups. The first
primitive is now implemented and independently tested; see [the standalone ledger project](../research/ledger/README.md).
Optional framebuffer pixel/coverage instrumentation and failure-injection tests now exercise real
allocation lifetimes. Frontend wiring and guards remain unchanged. The original proposal below is
the design basis; see the continuation handoff for current implementation scope.

Research design, not an implemented ledger or budget decision. Based on committed runtime
`52ccb33`, inspected on Linux x86-64. Atlas catalog/selection work changes no allocation path.
The owner's main worktree has unfinished renderer phase work. The initial proposal avoided shared
renderer headers, platform files and CMake integration; the continuation adds only optional
framebuffer accounting in its isolated branch, leaving `render.hpp`/`render.cpp` WIP and root CMake
untouched.

## Counted quantity and ownership

Count requested successful dynamic allocation payload bytes, once, at the allocation's owner.
Do not call that process RSS, heap overhead, application partition use or total resident memory.
Report embedded/read-only pack bytes and inline/static object storage separately. Borrowed pack
views add zero owned payload; an owned file buffer counts once even when multiple views borrow it.
Published and candidate pack allocations coexist during replacement and both count until release.

The inspected sites explain why existing `bytes()` methods cannot simply be summed:

- `Framebuffer::byte_size()` counts RGB565 pixels; its separately allocated coverage bitplane is
  `ceil(width/8)*height`. A line framebuffer has the same two allocations. Assign its allocations
  to line workspace rather than also counting them as the primary framebuffer.
- `Scene::create()` allocates `Vec3[point_count + graticule_points]` and `PolylineCap[polyline_count]`.
- `TerrainScene::create()` allocates world/projected corners, cell bands, corner values, cell
  passes and cell classes. `TerrainScene::bytes()` additionally includes inline color arrays;
  `required_bytes()` includes a 3,072-byte inline-color allowance. Those are useful logical cache
  costs but not the same quantity as heap payload. Do not count inline arrays twice.
- `RefinementJob` owns optional line-layer pixels/coverage and a `uint32_t` line list. Constructor
  allocation is distinct from a render-step allocation; measure both boundaries honestly.
- Decoded land classes and owned input buffers count separately. Embedded GELV/atlas borrowed
  bytes remain present even if a derived cache is destroyed.

## Proposed portable interface

A first implementation can live in one independent header plus a small test executable before
any call sites change. Proposed names/signatures (subject to review):

```cpp
enum class MemoryCategory : uint8_t {
    Framebuffer, Scene, LineWorkspace, TerrainCache, ClassCache,
    OwnedPack, JobWorkspace, Count
};
struct MemoryCounter {
    uint64_t current, peak, successful_allocations, failed_allocations;
};
struct MemorySnapshot {
    MemoryCounter categories[size_t(MemoryCategory::Count)];
    uint64_t total_current, total_peak;
    bool accounting_error;
};
class MemoryLedger {
public:
    bool acquired(MemoryCategory, uint64_t bytes) noexcept;
    bool released(MemoryCategory, uint64_t bytes) noexcept;
    void allocation_failed(MemoryCategory) noexcept;
    MemorySnapshot snapshot() const noexcept;
    void reset_peaks() noexcept; // peaks become current; never erase live bytes
};
```

All storage is fixed-size, zero-initialized and caller-owned. No string, vector, clock, mutex,
platform type, allocator hook or JSON formatting belongs in this type. Single event-loop owner;
thread safety is explicitly outside this first slice. Snapshots copy values without allocation.
The ledger must outlive allocation-owner tickets. Move-only tickets transfer accounting ownership
without reacquiring; move assignment releases the destination ticket first; moved-from destruction
is inert. Accounting category/size is immutable while a ticket owns an allocation.

`acquired` is called only after allocation succeeds. Checked arithmetic detects category/total
uint64 overflow; `released` detects underflow. Invalid categories and inconsistent releases set a
sticky `accounting_error`, return false, and leave numeric counters unchanged. Do not silently wrap
or saturate believable numbers. Failure counters also need checked arithmetic. A ledger error
means measurement is unusable; the first slice must not free memory or change render behavior.

Total peak is updated on each successful acquisition from the simultaneous total. It is **not**
the sum of independent category peaks. Example: allocate/release 100 framebuffer bytes then
allocate 200 terrain bytes: category peaks sum to 300, simultaneous total peak is 200. Partial
construction failure releases each successful allocation in reverse ownership order; only the
failed attempt increments its category's failure counter. Reserved estimates and attempted bytes
must never appear as successful live bytes.

## Diagnostics envelope and target boundary

A later frontend serializer supplies schema version, revision and dirty state, edition/pack hashes,
OS/version/architecture, compiler/build mode, pointer width, relevant `sizeof` values, viewport,
render strategy, grid sizes, counting scope and explicit exclusions. Use decimal fixed-width counts
and explicit units. Report measured counted payload, estimated inline/static bytes, process RSS,
and OS 9 partition/free-block observations as different fields with different origins. Record
latency/load timing from existing platform-provided clocks; the ledger never reads time.

The host and G4 should agree where allocation geometry and element layout agree. Do not promise
identical bytes for pointer-containing structures across ABIs. ABI differences must be explicit;
no target measurement is supplied here. Claude owns Classic Memory Manager/partition observations
and platform diagnostic serialization. Atlas estimates can adopt the measured categories later.

## Smallest next implementation issue

Proposed child of #55: **Fixed-storage accounting primitive and move-only ownership ticket**.
Scope: standalone portable type, checked arithmetic, snapshot/peak/failure operations and ownership
unit tests. No allocation-site migration, guard replacement, presentation change or pack format.
Acceptance: exercise overlapping/disjoint category lifetimes, reset while live, partial failure,
zero-byte operations, overflow/underflow, invalid category, move construction/assignment, and verify
zero allocator calls inside all ledger operations with an independent allocation counter.

Next slice after review: instrument framebuffer pixels/coverage and one scene constructor,
including failure injection and multiple simultaneous instances. Preserve current guards until all
relevant allocations and ownership are accounted for and old/new figures reconciled. This first
slice does not satisfy #55's full acceptance; #56/#57/#60 remain dependent and unimplemented.
