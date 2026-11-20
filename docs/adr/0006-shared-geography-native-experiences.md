# ADR 0006 (proposal): Shared geography, shared semantics, native experiences

**Status:** Proposed — owner-endorsed direction recorded 2026-11-18; not yet accepted or merged  
**Date:** 2026-11-18  
**Related:** [ADR 0002](0002-rendering-shared-code-and-usability.md), [ADR 0005](0005-entity-identity-and-navigation.md), #3, #83, #114, #148, #176, #183

## Context

Poodle Earth began as a Classic Macintosh application, but its portable C++20 engine, offline atlas, and Elm-style `Model / Msg / update / Effects` architecture make other systems plausible. Candidate experiences include lightweight Unix/Xlib, FLTK on supported desktop operating systems, historical Win32, web/mobile, and experimental consoles.

These environments differ in memory and CPU budgets, input devices, native window systems, user expectations, and historical interface conventions. Requiring identical UI features and window arrangements everywhere would increase porting and maintenance costs and could compromise the smallest targets. Conversely, arbitrary divergence in geographic identities, data formats, or command meanings would fragment the product.

## Decision

**Shared geography, shared semantics, native experiences.**

1. **Shared geographic foundation.** Preserve one portable geographic engine, canonical entity identities and navigation semantics, documented/versioned compatible atlas resource formats, and reproducible atlas production. Platforms may expose or load different levels of geographic detail because of real resource limits; they must not invent incompatible identity or pack semantics.
2. **Semantic consistency, not identical controls.** Where supported, operations such as zoom, select, navigate, Back, and changing a look retain consistent meanings and authoritative model state. Mouse, keyboard, controller, touch, menu, or scripting input can map to the same semantic actions.
3. **No universal UI-feature or layout parity.** Different frontends may intentionally offer different window arrangements, native widgets, menus, input devices, tools, and auxiliary workflows. Missing a frontend-specific convenience is not automatically a defect in every other frontend. Document which capabilities are supported or deliberately excluded.
4. **Platform-appropriate UX.** Design for each environment's practical and historical conventions. A Classic Mac multi-window workspace does not mandate multiple windows on a lightweight Unix Reader, mobile browser, or game console.
5. **Minimal Unix Reader remains minimal.** Retain Xlib as a small, low-dependency Unix frontend and portability/reference implementation, with essential geographic navigation and supported atlas access. Do not make controller support, FLTK, SDL, OpenGL, or rich workbench widgets prerequisites for Xlib. Existing Xlib scripted rendering and input tests remain valuable.
6. **FLTK is an additional workbench candidate, not a replacement mandate.** Its prototype may offer richer native desktop controls and optional controller integration on supported hosts; do not assume a particular Windows version, Mac SDK, or Unix target is supported until verified. Coexistence with Xlib is intentional while evidence supports maintaining both.
7. **Share proven behavior, not a generic GUI.** Reuse `Model / Msg / update / Effects`, existing command and keyboard contracts, `RefineRunner`, `ShownFrame`, information/card logic, and narrowly justified pure presentation helpers. Leave native event loops, window management, timers, clipboard, platform fonts, pixel submission, and toolkit-specific behavior in frontend adapters. Do not introduce a universal widget/layout DSL, duplicate geographic state, or a large coordinator to remove trivial duplication.
8. **Capability evidence is explicit.** Maintain a small, current per-frontend capability and acceptance record, distinguishing proposed, implemented, tested in emulation, and verified on target hardware. Shared geographic correctness is mandatory for supported operations; identical pixels or UI layouts across different native font/toolkit systems are not.
9. **Portability without automatic commitments.** A new frontend or controller experiment does not authorize releases, installer work, CD inclusion, device access, or user-facing platform support promises. Prefer maintained, reproducible, free-software build toolchains where practical.

## Illustrative frontend directions (not release requirements)

| Frontend | Intended character | Illustrative UI distinction |
| --- | --- | --- |
| Classic Macintosh Toolbox | Native Macintosh geographic workstation | Globe, Information, and Looks/Tools auxiliary windows (#148) |
| Xlib | Minimal Unix Reader and low-dependency portability reference | One compact window is acceptable; no controller obligation |
| FLTK | Portable desktop workbench | Richer tools, auxiliary windows and potentially optional gamepad input |
| Web / mobile | Responsive, browser-based Reader | Touch gestures and panels rather than desktop window parity |
| Game consoles | Controller-first geographic explorer | Full-screen navigation rather than desktop widgets |

The last two rows are research directions, not assigned frontends or supported platforms. The FLTK row does not require identical UI or compatibility on every operating system FLTK may target.

## Consequences and tradeoffs

- New geographic features should be expressed through shared semantic operations and resource contracts where feasible; platform-specific presentation is allowed.
- Documentation and acceptance tests need to distinguish **semantic/data requirements** from **optional interface capabilities** and from **supported-platform claims**.
- Some presentation code may be duplicated deliberately. Evaluate the third-frontend experience under #114 before extracting additional shared abstractions; the current accepted [ADR 0002 D6](0002-rendering-shared-code-and-usability.md) remains unchanged.
- The **Classic Mac multi-window decision** in #148 remains in force for Classic Mac. This ADR narrows only any *cross-platform interpretation* of identical window-layout parity; it does not reinstate an embedded-sidebar mode for Classic Mac.
- The older [product-direction brief](../design/product-direction-2026-11-10.md) contains historical single-window preferences that were superseded by the owner's 2026-11-13 Classic Mac workspace direction and this 2026-11-18 cross-platform principle. Preserve those dated decisions rather than silently erasing them.
- Future frontend changes may require capability-matrix updates, not automatic feature backports.

## Scope and adoption

This proposal documents product and architecture intent only. It does not change source, tests, an existing accepted ADR, milestone scope, release gates, or issue readiness. Review and explicit acceptance are separate owner decisions. Existing implementation and research assignments retain their boundaries.
