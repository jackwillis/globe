# Coarse tail continuation freeze — 2026-11-18

Owner-authorized incremental budget only, baseline8951cec3/numerical7ab358f2 unchanged.
Exactly view5-r220-relief, indices4138..4799 row-major, target1/2560. No candidates/scoring.
Original archive SHA256 d198144721dc036020ac0332c25499eececb312e33fce76fd88d7eb58555649b
and every archived evidence entry verified, original4138payload hashes/exact admissions pass.
Original sources, payloads, logs and analytical outputs are read-only dependencies.

Hard incremental ceilings300aggregateCPU seconds,600elapsed,384MiB/process,1GiBcombinedoutput.
Fixed allocations: preparation50CPU(including initial unmetered10),validation/reporting30,
supervision10,worker210. These are reserved allowances, not measured CPU.
Worker before-exec limits208soft/210hardCPU,clean205,alarm265wall/clean255.
Supervisor worker-wall270, aggregate elapsed500 from immutable start anchor, leaving70 seconds
plus initial elapsed30 below600. ParentCPUsoft9/hard10;384MiB limits inherited before exec.
Output storage checked at preflight and after run; per-write guard reserves headroom below1GiB.
Exactly one worker, one authorized attempt; no automatic restart or further resource request.
Original accounting range carried separately, never reset by this additional authorization.

Durable CPU ledger uses flock, fsync temporary file, atomic replace and directory fsync.
Reserve entire worker cap BEFORE launch, and retain that full charge if terminal telemetry is
lost. On wait/reap, record entire child user+system CPU including startup/setup/serialization,
then settle rounded-up measured charge. Supervision/preparation/reporting measured components
remain separately labeled within their reserved allowances. Live /proc telemetry is informative,
never substituted for a terminal total. Reopening ledger cannot reset reservations or cap;
repeat worker name is rejected. If aggregate budget exhausted, no worker starts.
Exception-safe finally signals/terminates/waits child BEFORE output collection and ledger settlement.
Missing atomic-temp paths do not disrupt accounting. A corrupt or interrupted output manifest
cannot upgrade partial bounds. Validate full tile before atomic certificate then atomic manifest;
reject preexisting output certificate. No old certificate rewritten/recomputed.

All failure-injection controls must pass before launch: supervisor exception, interrupted worker,
missing temp, interrupted collection, abandoned reservation/exhausted/restart budget, partial
manifest, truncated/false-complete/invalid-bound/invalid-quantization tile rejection.
These controls are operational; no geometric semantics/predicates/precision/time/cell/depth change.
Retained adapter calls unchanged engine:100000cells/pixel,depth16,75/64seconds/pixel, opaque order.
Source/input freeze written before geographic continuation. Stop/report even if incomplete.

Pre-geographic launch control exposed inherited hardCPU10 preventing child210 limit. No geographic worker started. Failed preexec charged conservative1CPU second (not invented measured total); frozen repair uses parent soft9/hard220 before exec, hard10 after childlaunch, child209soft207/clean205. Thus failed1+worker209+prep50+report30+sup10 still300. High-child-limit launch control passes before geography. Original freeze/failure preserved under launch-failure. Per-file cap4MiB, prior combined output<128MiB and tail payload cap128MiB keep evidence below1GiB.
