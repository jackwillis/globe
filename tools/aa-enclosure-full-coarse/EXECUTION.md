# One coarse full-fixture freeze

2026-11-18, owner-authorized feasibility only. Baseline7ab358f2 (local same-tree7f30f5a).
Exactly view5-r220-relief,640x480,4800tiles in row-major order: index=80*ty+tx.
Target1/2560 midpoint radius. No candidate output reads or scoring.
Retained adapter invokes unchanged scale-v1 Geometry.tile.100000cells/pixel, depth16,
75/64seconds/pixel, conservative wider bounds on failure; no quota transfers or retries.
Original four anchors are checked twice at original precision for exact endpoint equality;
synthetic mixed-tile reuse, original analytical and scale-v1 reliability/mutation controls pass
before execution. Those anchor controls are not a fine full-frame reference.

Owner hard ceilings1800CPU seconds including preparation/orchestration,2700elapsed,
384MiB address space/process,1GiB persistent output, one reference worker.
Operational sublimits:300CPU seconds preparation allowance, worker CPUsoft1445/hard1450,
worker clean stop1440, worker elapsed2080clean/2090alarm, supervisor2100wall and
absolute budget-start+2370wall guard; leave remaining allowance for reporting/persistence.
The budget-start follows initial read-only investigation; elapsed reserve330seconds also
covers that investigation and final reporting. The operational sublimits may stop before
owner ceilings. Tests accounted with child rusage; prior brief read-only/tool setup CPU is
not retrospectively claimed as directly metered and is covered conservatively by allowance.
Supervisor polls0.5s;384MiB address-space hardlimit on worker; supervisor is also launched
with384MiB address-space bound.950MiB output supervisorstop,900MiB worker tilepayloadguard
leave headroom below1GiB. No production processes or hardware are touched.

Every completed pixel atomically updates inflight.json(complete:false). Complete64-pixel
bounds are gzip serialized with exact integer endpoints, fsync'd, atomically renamed,
then appended to fsync'd completed.jsonl with SHA256. Only validated64-pixel tile files count
as completed; manifest/checkpoints never upgrade a partial tile. Precision-limited completed
tiles are distinct from precision-attained tiles. Inflight file may remain stale after a
completed tile; recovery checks index/hash and ignores stale entries. Hard interruption may
leave a .tmp, never a valid completed file. Resume is not automatically authorized.

Inputs permitted only frozen geometry/background. Source and input hashes are frozen in
source-freeze.json before the first whole-fixture tile. Geometry/BVH constructed once.
Report measured setup,tile,checkpoint/orchestration overhead, CPU/wall/RSS/storage,
precision state, unresolved area,predicates/cells/depth and discrete RGB565 ranges.
Comparison uses preserved scale-v1 cost model; no outcomes inform selection or stopping.

## Recorded operational repair, before continuation

Supervisor1 statted a temporary path after atomic rename and raised FileNotFoundError.
The worker was terminated with1326 complete tiles and48 completed pixels of tile1326.
Terminal worker rusage is missing; do not invent it. interruption1 retains the original
log, checkpoint and incident. A conservative phase1 CPU upper bound uses elapsed wall
since budget-start (single-thread worker, earlier than launch). Phase2 CPUsoft995/hard1000,
clean990, plus300 allowance and that recorded upper bound stay below1800. Absolute elapsed
budget guard remains unchanged. No numerical producer, target, traversal or quotas changed.
Supervisor tolerates disappearing temporary files. Continuation verifies every completed
payload hash/complete flag and skips exactly0..1325; only interrupted tile1326 is recomputed.
Its original partial bounds remain separately retained, not upgraded to complete certificates.
This is recovery within the same owner-authorized experiment, not a second fixture/run or
relaxed resource envelope. This operational repair supersedes the initial no-retry statement
only for the interrupted tile and is fully recorded before continuation. No scored protocol
is amended. run.py initial version remains in the source freeze and git history.
