# PE-JAVA — Historical JVM compatibility scientific protocol (proposed)

**Status:** Proposed protocol, distilled from Scientific Infrastructure response dated 2026-11-19. **No experiment authorized or run by this document.** Research-only tracking: [#213](https://github.com/jackwillis/poodle-earth/issues/213). Empirical context, vendor/API history and licensing: [historical-java-compatibility.md](historical-java-compatibility.md). General existing experiment registry and method: [rendering-experiment-program.md](rendering-experiment-program.md).

The objective is **not** to vindicate Java or build a globe first. It is to answer, with bounded evidence: *What exactly can be claimed when one Java artifact is said to run on a particular historical computer?*

## Claim ladder — never infer the next gate from a lower one

| Gate | Claim | Evidence |
| --- | --- | --- |
| 1. **Source** | Source parses/compiles under intended language rules | Exact compiler/build/source options and diagnostics |
| 2. **Binary and API** | Class files load and required API references resolve | Version audit of every class, independent signatures, authentic runtime linkage |
| 3. **Operational** | Same artifact launches, draws, responds and exits | Real OS/JVM/ISA/graphics execution and behavior tests |
| 4. **Geographic** | Independently interprets Poodle Earth's data correctly | Strict GATL fixtures, separately specified mathematical/format oracle |
| 5. **Supported product** | Is installable, documented, maintainable and tested | Release process and per-target qualification, **outside initial PE-JAVA research** |

A Java 1.4 class file (typically major version 48) can still call an API absent on 1.4; static API checking cannot establish AWT graphics; an Earth-like image cannot prove geographic correctness; one working Tiger JVM cannot qualify Windows or Solaris.

## Hypotheses and discriminating observations

| ID | Hypothesis | Prediction | What weakens or falsifies it |
| --- | --- | --- | --- |
| H1 | Correct compiler target + historical API check predict basic Java 1.4 binary portability | Positive probes load across distinct authentic JVMs | Complete static checks pass but a real target cannot link for a defect checks were supposed to catch; classify environmental failures separately |
| H2 | Vendor-specific runtime graphics behavior limits nominal compatibility | Identical JAR launches but input/paint/color/idle behavior differs | A defined small AWT workload behaves consistently across tested implementations (does **not** prove universal portability) |
| H3 | JVM/AWT baseline costs may invalidate low-resource usefulness | Startup, resident memory or pauses are disproportionate before meaningful geometry | Target-specific footprint and response leave usable margin under preregistered constraints |
| H4 | A limited independent Java Reader can achieve useful incremental reach | Same JAR parses pinned GATL and responds on multiple historical targets | Workarounds, memory cost or duplicate-engine maintenance exceed its bounded product benefit |

**Java 1.1/MRJ is a separate research question**, potentially demonstrating same Java 1.1 JAR on Classic Mac and another independent historical JVM. Do not silently broaden Java 1.4 work to an OS 9 compatibility shim.

## PE-JAVA-00: static + authentic binary compatibility protocol

**First research gate; scientific response recommends read-only protocol specification before any coding.** Its result may be either a reproducible public method or a justified statement of why the method is not yet possible.

### Original, tiny test corpus

1. **Positive A — Java 1.4 basic code**: standard language and library usage; expected compile + allowed class versions + signature pass + runtime load/exit.
2. **Positive B — graphics references only**: simple AWT/Java2D and BufferedImage *linkage*; static pass is not a claim of visual correctness.
3. **Negative A — post-1.4 API**: an explicit \`System.nanoTime()\` call; expected API signature rejection even if a modern compiler accepts it.
4. **Negative B — vendor-only API**: call a nonstandard implementation class; expected generic signature rejection or explicit nonportable classification.
5. **Optional reflection example**: demonstrate how runtime-generated class names can escape static detection; record limitation, not a fictitious pass.

Build with pinned JDK, explicit \`-source\`/\`-target\` and suitable historical \`-bootclasspath\` or independently audited API checks. Record every emitted and included class-file version; transitive JARs must not be assumed compatible. Prefer existing FOSS Animal Sniffer signatures/Ant support; audit their artifact provenance and licensing first. Write a custom checker only for a **demonstrated**, decision-relevant gap. A narrow independent major-version auditor is an optional separate tool, not a complete JVM verifier.

### Minimal runtime validation

Run the *exact same JAR checksum* on at least one authentic, identified historical Java 1.4 runtime. A second independent JVM tests cross-environment portability. Record vendor/build, OS/architecture, graphics backend, input and expected output, exit code, JAR SHA-256, dependency identity, errors and provenance.

**Do not attribute performance differences uniquely to JVM vendor** when CPU, OS and graphics stack also changed.

### Offline reproducibility and artefact classification

A **public** recipe can include original fixtures/source, build commands, dependency checksums, permitted signatures, expected output, and results. Proprietary runtime binaries and substantial extracted API lists remain **restricted local inputs pending rights review**. Document inability to reproduce a restricted input openly. Do not upload restricted binaries to shared CI or silently treat archive accessibility as licensing permission.

Suggested provenance fields: research ID, vendor/JVM build, OS/ISA, acquisition source/date, archive hash, authentic install vs reconstruction, EULA/license evidence, publication classification, extractor/method, tested artifact hash, result, omissions.

## PE-JAVA-01A: same-JAR AWT operational pilot (conditional)

Only after a successful static/binary method and an owner-approved pilot scope. Use **one original deterministic pixel-pattern program**, not geography:

- One fixed-size AWT window; CPU-generated \`int[]\` / BufferedImage pixels; simple repaint, input and clean shutdown.
- Use a single JAR on Apple Tiger 1.4.2 and, if actually available and legitimately acquired, a second materially different Java 1.4 JVM (e.g. Windows or Solaris).
- Check image orientation and channel order, repaint after exposure/resize if supported, event direction/order, stable idle CPU, no required network, no unexpected writes, exit correctness.
- Describe startup, memory and presentation timing boundaries honestly. Completion of \`paint()\` is not optical on-screen latency.

A second runtime is *not* assumed to exist just because an OS historically ran Java. Cross-platform launch success is a tested-configuration claim, not vendor causality or a supported release.

## PE-JAVA-01B: smallest GATL Reader (conditional)

Only after PE-JAVA-01A is technically credible and incremental platform reach remains important.

- One pinned **synthetic GATL v1** fixture, no Java-only geographic format.
- Strict parser with bounded allocations and error handling.
- One simple globe projection; original software pixel generator; rotation/viewpoint input.
- No networking or JNI, and no labels, cards, city search, scripting, regions/editions or media.
- Reuse **semantic** reference contracts, not the C++ implementation or proprietary JVM internals.

### Independent reference correctness

**Format:** separately specified valid record, truncated header/payload, CRC failure, invalid version/count, out-of-range latitude, endian handling.

**Geometry:** independent known front/back hemisphere and equator/pole cases, antimeridian, horizon-adjacent segment, rotation direction and projection expectations. Have a human-readable mathematical oracle or independently derived values, not just agreement with C++.

**Rendering/input:** exact pixels only for precisely specified operations; for FP projection differences, define justified tolerances *before examining failures*. Never widen a threshold solely to make a failing implementation pass. Categorize format, semantics, rendering and responsiveness distinctly.

## Performance protocol (if PE-JAVA-01A/01B authorized)

Four workloads separate JVM initialization, graphics, geometry and combined overhead:

1. Launch minimal class with no AWT.
2. Open AWT and present a constant framebuffer.
3. CPU render without displaying.
4. CPU render **plus** AWT present.

Record cold-start/readiness and first-buffer events, first completed display call, warmed render, input-to-presentation, idle CPU, heap used, RSS, allocation rates, GC frequency/pauses, worst observed stall, JVM JIT/interpreter and warmup state. Record timestamp boundaries precisely.

Hold scene, size, color depth, format, input trace, quality and build as matched as possible. Compare native C++ and Java **on the same OS/hardware** where feasible; a Classic OS 9 C++ run versus Tiger Java run on the same G4 is not an isolated language effect. Predefine workable memory/latency goals from real target constraints and the measured baseline, rather than inventing a Java heap limit.

## Stop/go rules and ownership

**PE-JAVA-00 succeeds if:** the source/bytecode target is documented; positive/negative signatures discriminate; at least one authentic Java 1.4 runtime loads a passing artifact; method+limits are documented; publication restrictions are explicit; no prohibited redistribution is required.

**PE-JAVA-00 ends inconclusively or stops** if the compiler/API validation is not credible, necessary publication rights cannot be accommodated through the private-input/public-recipe split, or target runtime identity cannot be established. A missing second runtime is **inconclusive cross-vendor evidence**, not demonstrated Java incompatibility.

**Advance to 01A only if:** PE-JAVA-00 works, a historical runtime exists for controlled testing, a second target is plausibly obtainable under acceptable conditions, and incremental reach is worth the effort.

**Advance to 01B only if:** the same JAR works on multiple relevant environments (or owner explicitly values a narrower single-target experiment), operational behavior and resource costs are plausible, and a format+independent oracle is available.

**Maintain a Java Reader only if:** real useful capability, target-native compatibility, resource margin, duplicated-code cost and incremental reach are demonstrated **and the owner explicitly accepts long-term maintenance**.

Negative results close tested hypotheses; they are not invitations to widen the experiment until a positive result appears.

**No authorization conveyed here:** No JDK acquisition, source change, JAR build, G4 access, issue readiness change, release commitment, publication of proprietary inputs, or new tool implementation. Scientific Infrastructure owns method/evidence proposals; Platform Engineering retains native packages and release work; the owner controls each next gate.