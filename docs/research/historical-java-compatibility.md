# Historical Java compatibility research — Java 1.4 and Java 1.1

**Status:** Archived literature/tooling investigation plus *attributed, not independently reproduced* historical class-file observations, 2026-11-19. **Research only.** Companion scientific protocol: [PE-JAVA](pe-java-experimental-protocol.md). Archive issue: [#213](https://github.com/jackwillis/poodle-earth/issues/213). No Java Reader, build system, JAR, historical JDK or support claim is added by this document.

## Research question and architecture boundary

Could **one Java bytecode Reader** use the published Poodle Earth geography on several historically distinct desktop systems while leaving the **native C++20 core, Retro68 Classic app and Xlib/FLTK work** canonical? This is not a proposal to rewrite the production application or to run Java inside the C++ engine.

Java is an implementation strategy, **not** an operating system/ISA target; it would need real JVM and AWT evidence for each exact OS/architecture/vendor combination. Possible historical subjects include Tiger PowerPC, Solaris SPARC/x86, Windows XP/98-era JREs, and vintage Linux/GCJ implementations. Windows XP itself should not be assumed to supply a suitable JVM. No target runs the hypothetical Reader yet.

The earlier illustrative full-Java rewrite scored **3.75 RICE** using reach 50/quarter, impact 1.8, confidence 0.25, effort 6 person-months. These values were speculative; a small compatibility experiment must be evaluated by **value of information**, not assigned that rewrite score. See [the archived RICE model](platform-rice-2026-11.md).

### Java 1.4 or 1.1?

| Dimension | Java 1.4 | Java 1.1 |
| --- | --- | --- |
| Proposed role | First product-relevant *alternate Reader* feasibility | Separate historical same-class-files portability experiment |
| Historical desktop possibilities | Apple Tiger, Sun/IBM JREs on Windows/Linux/Solaris if individually verified | Apple MRJ/Classic Mac plus other old desktop JVMs if individually verified |
| Rendering | AWT/Java2D, BufferedImage, primitive int[] CPU framebuffer | AWT; no standard Java2D, consider MemoryImageSource |
| Standard API availability | Collections, regex, NIO, Java2D | Many later APIs absent; more hand-written support |
| Poodle Earth purpose | Complements native Classic Mac OS | Overlaps existing native Classic Mac OS |
| Status | Suggested first research target, **untested** | Deferred, independently testable research |

The original Claude memo described MRJ 2.2.x/Java 1.1.8 and Tiger Java 1.4.2 partly from historical recollection. Verify exact system/runtime builds before calling them accepted target configurations.

## Source records, attribution and scope

**Claude (Sonnet 5.5), "Memo: Java 1.4 / 1.1 front end research for Poodle Earth", 2026-11-19**: examined a checkout at \`cfa07a8\`, inspected class libraries provided by the owner, ran a temporary Python class-file parser **outside the repository**, and prepared a private API text inventory. Its original markdown was supplied to the Platform Portability thread; local SHA-256 there was:

    162e32bdf1ab352d0b744cae07c536001ac1df50632e78da70055588ccc28ef4

This document **does not contain** that input archive, extracted full inventory, or a reproducible parser. The numbers below remain **Claude-reported, not independently replicated** by this archival work.

The follow-up **Scientific Infrastructure and Experimental Methods** response dated 2026-11-19 supplied the hypothesis/control/stop-go design; its supplied transcript's observed SHA-256 was:

    c41904915973b187f85ff6efc222013b1a6e2840bcbd5f2973c0dd9d5fc6164d

See the [condensed, explicitly proposed protocol](pe-java-experimental-protocol.md). Those hashes identify the exact locally provided source *copies* observed during archiving; they do not establish the origin or redistribution rights of the underlying JVM archives.

## Claude's Apple Tiger Java 1.4.2 library examination

The owner supplied a \`classes14.zip\` archive described as eight JARs from the Tiger G4's Java 1.4.2 installation (~18 MB). Claude reported:

| Measure | Reported count |
| --- | ---: |
| Class files parsed | 11,964 |
| Parse failures | 0 |
| Public classes | 7,437 |
| Text inventory of public/protected API | ~81,000 lines |
| Class files at version 48.0 | 11,130 |
| At 46.0 | 583 |
| At 45.3 | 251 |

Per-JAR counts: \`classes.jar\` 10,521; \`charsets.jar\` 423; \`jsse.jar\` 380; \`laf.jar\` 252; \`ui.jar\` 196; \`sunrsasign.jar\` 89; \`jce.jar\` 60; \`dt.jar\` 43.

Selected **present** API observations: \`Math.toRadians\`, \`String.split\`, \`Arrays.sort\`, \`Math.round(double)\`, \`LinkedHashMap\`, \`java.nio.ByteBuffer\`, \`java.util.regex.Pattern\`, \`BufferedImage\`, \`Graphics2D\`.

Selected **absent** references: \`StringBuilder\`, \`System.nanoTime\`, \`Math.hypot\`, \`Math.cbrt\`, \`Math.log10\`, \`Math.sinh\`, \`String.format\`, \`String.contains\`, \`Integer.valueOf(int)\`, \`java.util.concurrent\`, \`java.util.Formatter\`.

The Apple libraries include vendor-specific \`apple/*\`, \`com/apple/*\` and other internal namespaces, with graphics code spread across multiple JARs. Claude reported approximately **3,680** public classes in internal/vendor namespaces versus approximately **2,520** in \`java/*\` and \`javax/*\`. These are **not equivalent definitions of Java 1.4 compliance**. A portable implementation must avoid undocumented Apple/Sun internals and test each target's actual runtime.

**Crucial distinction:** the version of JDK class files found inside a JVM is not the version of class files the Reader must emit; a class-file version alone is not an API guarantee.

## Tooling findings and open verification

- **JDK 8:** Oracle's historical \`javac\` documentation describes \`-source 1.4\`, \`-target 1.4\`, \`-bootclasspath\` and cross-compilation. A concrete JDK 8 installation and its exact warnings/output have **not** been run as part of this research archive. Newer JDKs do not generally emit Java 1.4-compatible bytecode.
- **API mismatch trap:** compiling Java 1.4 bytecode against a modern runtime can still encode references absent on a Java 1.4 JVM and fail with linkage errors. The selected historical platform class library, or vetted API signature checking, is a separate requirement.
- **Animal Sniffer/Animal Scents:** the official historical signature catalog names \`org.codehaus.mojo.signature:java14:1.0\`, \`java14-sun\`, \`java14-ibm\`, and \`java14-jrockit\`. Confirm each precise artifact's provenance and notices before vendoring. The \`java14\` name means *Java 1.4*, not Java 14. Animal Sniffer supports API signature checks and has an Ant integration.
- **Custom Rust checker:** Claude originally proposed checking major version <=48 and inherited member references. The scientific response recommends **not** writing a new full checker before evaluating the existing FOSS implementation. A tiny independent version auditor might still have value.
- **GNU Classpath / GCJ:** potentially useful GPL-with-special-linking-exception historical sources; actual version, API completeness, AWT behavior and clean-room contribution requirements must be examined separately. This is not presumed identical to Sun Java 1.4.
- **JVM availability:** a historical Red Hat, Solaris or Windows media image may contain a runtime, but the OS name does not identify its JVM vendor, build or compatible AWT implementation.

References to independently examine and pin in PE-JAVA-00:

- Oracle JDK 8 javac: https://docs.oracle.com/javase/8/docs/technotes/tools/unix/javac.html
- Animal Sniffer: https://www.mojohaus.org/animal-sniffer/
- Historical signatures: https://www.mojohaus.org/signatures/
- GNU Classpath: https://www.gnu.org/software/classpath/
- GNU Classpath licensing: https://www.gnu.org/software/classpath/license.html
- GNU Classpath clean-room FAQ: https://www.gnu.org/software/classpath/faq/faq.html

## Proposed licensing and provenance boundary — NOT adopted legal policy

There are three materially different artifacts:

1. **Restricted source inputs:** complete proprietary JRE/JDK binaries and archive contents. Do not commit, upload to public CI/caches, or redistribute absent specific rights and owner approval.
2. **Derived vendor API catalogs:** substantial extracted class/member inventories. Do **not** assume full publication is permitted merely because the symbols are names/descriptors; keep locally restricted pending rights review.
3. **Original public research:** authored parsers, tiny original compatibility fixtures, aggregate counts, limited feature-test pass/fail tables, hashes, methodology, and only reference signatures whose individual licences/provenance permit redistribution.

Google v. Oracle (2021) is relevant to API interoperability, but is **not** a blanket license to publish complete proprietary runtime API databases. GNU Classpath's linking exception is **not** a license to copy arbitrary implementation code under new terms.

Proposed minimal provenance manifest for each historical runtime: stable research ID; vendor, JVM version/build; OS/ISA; acquisition source/date and whether from authentic installation vs repack; SHA-256 of restricted input; EULA/licence evidence; publication class (public/restricted/pending/prohibited); extractor version/method; test JAR SHA-256; outcome and known omissions. Treat source-archive provenance and derived-observation provenance separately.

**Licensing status remains unresolved.** An owner-approved rights review is required before distributing any historical proprietary JDK or complete vendor-derived API inventory. No new source acquisition is authorized by this record.

## Evidence stages and next research decision

Keep five gates distinct:

1. Source compatibility and compiler version/flags.
2. Class-file version **and** API/linking compatibility.
3. Real historical JVM operation and AWT graphics.
4. Geographic file-format and semantic interoperability.
5. Supported release: installation, dependencies, licence notices, platform-specific acceptance and maintenance.

PE-JAVA-00 should establish a **public recipe with private optional inputs**, known-positive and known-negative probes and a verified actual JVM launch; only then evaluate a graphical same-JAR pilot and a minimal GATL Reader. The [Scientific Infrastructure protocol](pe-java-experimental-protocol.md) defines the experiments and stop rules. There is no Java build, Reader, artifact release or target performance evidence yet.

**Ownership:** Scientific Infrastructure leads research methods; native product/release work continues separately. Hardware access, implementation, proprietary input handling and release changes require fresh owner authorization.