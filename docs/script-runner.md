# First Classic script runner (`pe-script/1`)

Prototype for [#221](https://github.com/jackwillis/poodle-earth/issues/221).
This is an ordinary application feature, independent of release gates. Native acceptance
is pending. Host tests and cross-compilation do not establish Classic runtime success.

## Using it

Choose **File → Run Script…**, select a text file, then choose **Run**, **Run and Save
Output**, or **Cancel**. Saving opens Standard File's Save As dialog before execution.
The window title reports the command count; **File → Cancel Script** or Escape in the
Globe window cancels. Escape is ignored by this handler when About, Preferences or a
desk accessory is frontmost; use File → Cancel Script in that case. Quit also cancels/finalizes.
The prior window title (including a timing title) is restored after completion/cancellation. A final alert reports status, command
count, assertion failures, errors, error code, and whether saving succeeded. No transcript
is retained in RAM when saving is off. Selection/confirmation/Save As cancellation runs
nothing. A script's state changes are not rolled back on failure or cancellation.

V1 creates only **new** output files. An existing destination is refused, even if Standard
File offers replacement confirmation. Choose another name and run again. This protects
scripts and distribution files without relying on replacement-dialog behavior. No output
path, preference, cache or hidden directory is chosen implicitly. An open/write/close/volume-flush
failure is reported; a failed run can leave an empty or partial user-selected file.

Suggested HFS filename: up to ten ASCII letters/digits/hyphens before the script's first
period (other bytes become `_`), `-`, eight lowercase hex digits of the Macintosh-epoch
local clock seconds, `.jsonl` (at most 25 bytes). Empty stems use `script`; unavailable/
zero wall time uses `00000000`. This is a naming aid, not a timestamp/provenance claim.
There is no automatic collision retry or suffix: exclusive creation fails deterministically.
The native dialog controls the actual name and destination.

## Grammar and limits

```text
get borders
set borders true|false
wait settled MILLISECONDS
assert borders true|false
```

The `|` denotes alternatives, not literal syntax. Case-sensitive ASCII tokens; spaces and
tabs separate tokens. One command per line, CR, LF and CRLF accepted. Blank lines and
lines whose first non-whitespace character is `#` are ignored. No inline comments.
Maximum **16,384 bytes per script**, **200 bytes per line**, including whitespace/comments
but excluding line endings. Timeouts are decimal digits, **1..60,000 ms**, with leading
zeroes allowed. Extra arguments, unknown commands, misspellings, invalid values, NULs
in commands and non-ASCII command tokens fail. Non-ASCII comments are permitted within the line bound.
The legacy trace parser/grammar and X11 `--script` have not changed.

[`tests/data/borders.script`](../tests/data/borders.script) contains the initial four-command
experiment. Source is read into one fallible, explicitly triggered Toolbox allocation,
then borrowed by the runner until terminal status. Commands are interpreted incrementally:
a later syntax error does not undo earlier operations. Stop on first error or failed assertion.

## State and completion boundaries

- `get` reads `Model.layers.borders` and returns a boolean (`evidence: model`).
- `set` compares actual state and dispatches the existing `CommandIssued/ToggleBorders`
  through `Application::apply` only if needed. It verifies the resulting state, returns
  `phase: applied` and `changed`, and makes no presentation-completion claim. An active
  refinement job holds a snapshot of Layers, so the script adapter cancels/restarts that
  job with current Model state. This differs deliberately from interactive B/menu border
  toggles: their existing path requests a redraw but does not replace an active job's Layers
  snapshot, so later bands can still reflect the prior setting. This PR does not repair that
  pre-existing interactive behavior; wait completion is never independent visual proof.
- `assert` reads current Model state again and returns `pass` or `fail`, with `observed`.
- `wait` captures the current Model generation and frontend command-redraw revision.
  It also captures the border value. A changed generation/revision/border value fails
  with `superseded`; it never silently follows a new view. Layer commands need the
  extra revision because existing Model layer toggles do not increment generation.

A wait succeeds only on a **later event-loop turn**, in the foreground, with settled
Model/refinement state, no active job or held presentation, no redraw/dirty pixels,
no quick overlays/progress bar, no pending snapshot/copy bands and no queued update event.
This means the application's required synchronous QuickDraw `CopyBits` submissions have
returned for that state. It does not prove correct pixels, scanout, native visual
verification, or hardware acceptance. A pixel-lock failure cannot become completion.

Time comes from the existing Microseconds-based monotonic adapter; core script code reads
no clock. Timeout wins if the deadline has been reached when completion is observed.
Script stepping is gated on foreground state, including non-wait commands. During
suspension the existing application scheduling policy yields until resume; elapsed
monotonic time still counts, and an expired wait fails when the app resumes. Modal menu/
window tracking also cannot deliver a deadline until control returns to the outer loop.

The runner consumes at most one source line or one pending-condition observation per turn,
including comments/blanks. There is one pending wait. Source scanning is bounded by the
16-KiB script limit. Script dispatch, serialization and file I/O occur outside the measured
rendering slice. `WaitNextEvent` gets the earliest rendering wake, script timeout, or **16-ms completion
observation wake** while a wait is pending (rounded up to 60-Hz native ticks). This
bounded observation policy also checks already-settled idle views without mouse events.
Waiting alone requests positive sleeps, not a zero-sleep busy loop; terminal runners
stop these wakes. There is no secondary/recursive execution loop.
The native file dialogs are modal only before execution. Existing OS calls, rendering,
full-frame conversion and disk I/O can still stall a turn; no hardware latency bound is claimed.

## JSONL contract

UTF-8, one complete object plus LF per record, schema `pe-script/1`:

| Record | Required content in v1 |
| --- | --- |
| `run` | schema, app version, build identity or `unavailable`, OS version or unavailable, platform/environment, script name, explicit absent checksum, save policy, initial borders, edition description/identifier availability |
| `command` | 1-based `seq`, physical source `line`, original input, typed result, evidence |
| `end` | `status` (`pass`, `fail`, `error`, `cancelled`), commands, assertions_failed, errors |

`commands` counts attempted non-ignored commands, including a parse failure or cancelled
pending wait. Assertions count only actual failed comparisons. Cancellation is distinct
from error: a cancelled wait has an error-shaped command with code `cancelled`, but does
not increment the error counter. Pre-read filesystem/resource failures run no commands
and show a UI error; they do not create a transcript. A shared-runner script-size error
has a synthetic error at sequence/line 0 (the native adapter rejects it before starting).

Result vocabulary: `value` (boolean), `ok` (`applied`/`changed` or
`completed`/`presentation-settled`), `pass`/`fail` (`observed` boolean), `error` (`code`).
Future `values` can be another result object without changing top-level framing.
Error codes include `malformed-input`, `resource-limit`, `unsupported-operation`,
`state-not-applied`, `timeout`, `superseded`, `invalid-clock`, `cancelled`, `io-failure`,
and `report-overflow-or-encoding`. Invalid/oversized input that cannot be safely included
uses `[invalid or oversized source line]` plus its exact line number.

Script filenames are converted from MacRoman to UTF-8 using a fixed table; selection of
a non-Roman-script filename is explicitly refused before execution; all other native
metadata is ASCII. The writer validates UTF-8 scalars (rejecting overlong sequences,
surrogates and out-of-range scalars), escapes quotes/backslashes and every U+0000..001F
control, and formats unsigned integer numbers without locale-dependent libc formatting.
V1's number API accepts unsigned integers only: floating-point inputs, including NaN and
infinity, are rejected at compile time. No floating-point result exists in this slice.
The trusted literal-writing primitive is only for fixed schema fragments, never user input.

Serialization uses a **4096-byte stack record buffer**, no dynamic JSON tree or heap
allocation, and a caller callback. Oversized/invalid records never reach the sink. One
complete record is submitted per write; a short/failed write stops execution and latches
report failure. No further record, especially a successful `end`, is attempted through
that failed sink. Earlier complete lines remain parseable; filesystem crash durability
is not promised. After attempting `FSClose` exactly once, the adapter calls
`FlushVol(nullptr, selected_volume)` once using the Save As reply's volume reference, even
if closing failed. Write, close and flush failures latch unsuccessful saving; the UI reports
`write-failure`, `close-failure` or `flush-failure` (first failure wins), status `error`, and
an error count. No default volume, periodic volume flush, or extra output is used.
`end.status` describes command execution; it precedes closing/flushing and cannot attest
that either succeeded. A pass record plus an unsuccessful final UI must not be called a
successfully saved report. Even successful File Manager flush is not a power-loss guarantee.
This follows Apple's [Inside Macintosh: Files, pp. 1-46 and 1-55](https://dev.os9.ca/techpubs/mac/pdf/Files/Intro_to_Files.pdf):
closing handles file data; volume flushing updates the volume/catalog metadata.
A missing valid newline-terminated `end` is **incomplete**, never pass.

Illustrative output (metadata here is explicitly fictional; no native run is claimed):

```jsonl
{"type":"run","schema":"pe-script/1","app_version":"example","app_build":"unavailable","os":"Mac OS version unavailable","platform":"classic-mac-powerpc","environment":"normal application; native/emulated host unavailable","script":"borders.script","script_checksum":null,"checksum_status":"unavailable","save_requested":true,"initial_borders":true,"edition":"embedded floppy; edition identifier unavailable"}
{"type":"command","seq":1,"line":1,"input":"get borders","result":{"type":"value","value":true},"evidence":"model"}
{"type":"command","seq":2,"line":2,"input":"set borders false","result":{"type":"ok","phase":"applied","changed":true},"evidence":"model"}
{"type":"command","seq":3,"line":3,"input":"wait settled 10000","result":{"type":"ok","phase":"completed","condition":"presentation-settled"},"evidence":"application"}
{"type":"command","seq":4,"line":4,"input":"assert borders false","result":{"type":"pass","observed":false},"evidence":"model"}
{"type":"end","status":"pass","commands":4,"assertions_failed":0,"errors":0}
```

Build identity is captured by CMake configuration (`git describe --always --dirty`), or
explicitly `unavailable` if the source tree's Git metadata is inaccessible (as in the local
container mount). Reconfigure before building after a revision change. Environment metadata
does not guess whether Classic is hosted by an emulator/Tiger. No checksum or unique
published geographic edition identifier is invented.

## Owner-run Classic acceptance (not executed by the agent)

1. Record candidate revision/checksum, OS, native/Classic environment, partition size and
   data edition. Launch normally from read-only distribution media; confirm startup and
   ordinary input/rendering still work. Do not change machine preferences for this test.
2. Put `borders.script` on readable media. Open Run Script; cancel each selection/confirmation/
   Save As stage in turn. Confirm border state unchanged and no output file created.
3. With borders enabled, Run without saving. Check title/progress, responsiveness and final
   counts (4/0/0); borders should be off. Repeat: set reports no state change when saving.
4. Run and Save Output to a fresh filename on a writable volume. Check MacRoman filename
   conversion, JSONL with an independent JSON parser, typed values, sequence/source lines,
   observed current state and final end. Check native dialog/default-button layout at the
   supported screen sizes. Try existing filename and read-only destination; nothing is overwritten.
5. With no mouse/keyboard input after confirmation, run the four-command script from an
   already-settled view; it must complete without waiting ten seconds. Repeat while a render
   is pending. Watch that it yields ordinary input between observations. Move another window
   frontmost: Escape does not cancel there; File → Cancel Script still does. Confirm prior
   title restoration and Run/Cancel placement before Quit.
6. Test misspelling `assert borderz false`, assertion failure, and oversized input: execution
   stops and later commands do not run. For a failed report write/full disk, close or volume-flush failure, verify the UI
   never says saved; a missing end is incomplete, and even a present pass end does not
   override a reported finalization failure. Use only expendable, explicitly selected output.
7. Begin a wait while rendering; exercise Cancel Script and Escape. Begin a wait, change
   camera or border state before it settles, and confirm `superseded`. Test a 1-ms timeout.
   Suspend/resume across a wait deadline; confirm timeout rather than stale completion.
8. Test set during active refinement and during a settled view. Compare actual border pixels
   independently; the script's Model assertion alone does not establish visual correctness.
   Record UI, responsiveness, filesystem and close-error findings on #221. No acceptance
   checkbox is granted by host or cross-build results.

No additional owner design decision is blocking review. The conservative no-overwrite policy,
integer-only writer and 60-second timeout ceiling are documented v1 choices to review.

## Owner-run stack observation plan (not executed)

Use the ordinary candidate and a native debugger only in a separately owner-authorized
session. Record candidate revision, OS/hosting environment and application partition.
`StackSpace()` is a read-only Memory Manager observation; do not change the stack/heap,
preferences or partition during sampling. First sample at Run Script entry (before dialogs),
then break in `ScriptSession::write` while its caller's `JsonRecord` is live, and sample again
at the completion alert. Debugger pauses count toward monotonic deadlines: perform these
samples separately from idle-wait correctness/timing runs. Exercise successful output, a failed assertion and an I/O failure
only on expendable user-selected output. Record each value and call site on #221; do not
label these few samples a proven minimum. Do not add serialization or file I/O to a measured
rendering phase. Static compiler frames are in the follow-up evidence note; they exclude
Toolbox internals and cannot replace native low-water observations. No `StackSpace()` native
observation or extra runtime instrumentation was performed by this agent.
