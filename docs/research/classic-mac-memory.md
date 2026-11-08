# Classic Mac memory: research note for #58 (with pointers for #55)

Research only; nothing here is implemented. **Verified** means declared in the installed Retro68
Multiversal `Multiverse.h` (checked 2026-11-08 in `ghcr.io/autc04/retro68`, which the repository's
`tools/build-os9.sh` uses) or measured in this repository. **Documented** means from the historical
Inside Macintosh Memory Manager reference and not yet confirmed on the G4 or in a header.
**Hardware** means it needs a run on bare OS 9.

## Facilities, and what is confirmed

| Facility | Header | Notes |
|---|---|---|
| `FreeMem`, `MaxMem`, `CompactMem`, `PurgeSpace` | verified | free bytes in the application heap; `MaxMem` also compacts and purges; `PurgeSpace` reports total and the largest contiguous block |
| `NewHandle`/`NewHandleClear`, `MemError` | verified | relocatable blocks; every allocation must check the result (`MemError` is the status, not a C++ exception) |
| `HPurge`, `HNoPurge` | verified | purgeable handles: the system may discard them; the owner must test `*handle == nullptr` and reload |
| `SetGrowZone` | verified | a callback invoked when an allocation fails; may free memory and return how much. Runs at an arbitrary point, so it must not allocate |
| `MoreMasters` | verified | master pointers for handles; call at start to avoid fragmentation by master-pointer blocks |
| `MaxApplZone`, `GetApplLimit`, `SetApplLimit`, `ApplicationZone` | verified | grow the heap to its limit once at startup; the limit is what the partition allows |
| `StackSpace` | verified | remaining stack; the application stack is small by default |
| `TempNewHandle`, `TempFreeMem`, `TempMaxMem` | verified | temporary memory from outside the partition (system heap free space). Documented: it can disappear at any moment and is released only by the app |
| `SIZE` resource | in `os9/globe.r` | requests 24 MiB preferred, 12 MiB minimum. The partition is the Finder's "Get Info" memory setting, which the user can override |

Do not assume more than that: for example `PurgeMem`/`CompactMem` calls do not by themselves reduce
what Globe allocates, and `TempNewHandle` memory may be lent to someone else's partition.

## Differences between native OS 9 and Classic under Tiger

Documented, not measured by us: native OS 9 has the real Memory Manager and the partition is real.
Classic under Tiger runs OS 9 inside a process whose memory is allocated by OS X, so `FreeMem` and
`TempFreeMem` describe the Classic environment, not the machine, and paging happens outside the
Memory Manager's view. Timing already differs (OS X layer, much slower); expect memory numbers to
differ too. **Hardware:** judge partition behavior and low-memory behavior on bare OS 9 only.

## What allocates in Globe today (candidates for a ledger)

From the code, not measured on the target: the framebuffer (`Framebuffer::create`), the GWorld at
16 bits (`NewGWorld`), the line layer and its pixel list in `RefinementJob`, `ZoomPreview` and
`ShownFrame` snapshots (frame-sized copies), the terrain scene (`TerrainScene::create`,
`terrain_cache_bytes` on X11 was about 18 MB), the atlas, countries and places scenes, and
`std::vector`s in the runner. C++ `new` in Retro68 goes through `malloc`, which sits on the
application heap. A ledger (#55) therefore needs to count: the frame-sized buffers (sizes follow the
window; a resize allocates the new buffers before releasing the old ones, so peak is two sets), the
GWorld (a Toolbox block, outside `malloc`'s accounting), the preview and shown-frame copies, and the
scene arrays; it should also read `FreeMem`/`PurgeSpace` for the real remaining heap, because
`malloc` failures and `NewHandle` failures are different kinds of failure. `new` in this build does
not throw (exceptions are off), so allocation failure must be a checked null.

## UX changes: size and memory cost

Measured by building with `tools/build-os9.sh` (PowerPC, not run for memory): the application's
data fork was **541,591 bytes** after the cursor change and **541,674 bytes** with the palette added
(+83 bytes; payload headroom against the 1.2 MB budget 621,013 -> 620,930). The palette's code is
compiled in (`LooksPalette` symbols are in the XCOFF), so the small delta is the measured number,
but it is smaller than expected and was not compared with a clean baseline build: both builds also
include two uncommitted rendering edits that were in the working tree. Treat the cost as "tens to a
few hundred bytes" until a clean baseline is built.

Runtime: the palette is one small window (a window record, a region and its port; a few hundred
bytes, estimated, not measured) created on first `P` press and kept; the cursor path adds a
`CursorTarget` and a cached pointer position, no allocation, and no framebuffer. Neither adds a
full-frame buffer. Neither is expected to change the partition, which is dominated by the
framebuffer, GWorld and terrain, but that is an estimate until measured with `FreeMem` on the G4.

## Suggested G4 acceptance tests (bare OS 9)

1. Launch with the default partition: read `FreeMem` after startup and after the first settle
   (a diagnostic title, #59).
2. Drag the partition below the minimum in the Finder: expect the system's own refusal, not a crash.
3. Resize the window to the largest size the limits allow; confirm no crash and that a failed
   allocation shows a graceful result (today `buffers()` failure quits).
4. Open the palette and the info card repeatedly, then compare `FreeMem` (leak check).
5. Run with other applications open until the partition is tight; look for beach balls, a crash or
   silent corruption (the failure symptoms to record).
6. Repeat the same steps under Tiger Classic and record how the numbers differ.
