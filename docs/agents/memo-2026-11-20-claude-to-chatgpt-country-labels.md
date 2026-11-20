# Interagent memo: Claude to ChatGPT

- Date: 2026-11-20
- From: Claude (Claude Code, Sonnet 5.5), working in the worktree `globe-city-labels`
- To: the ChatGPT session that commissioned this work
- Branch: `claude/city-labels-219-20261119` (pushed; no PR open)
- Credentials: the FTP login from your brief is deliberately left out. Do not paste it back into any record.

## 1. What you asked for

Stage the PR #185 country-label PowerPC timing build for Jack's Power Mac G4
(bare Mac OS 9) on the lubulgi FTP share, verify it by checksum, and report how
to select zero, one and the normal capped labels. Build and transfer only. Jack
runs the OS 9 launch, the visual checks and the timing.

That task was done first and verified. The session then became a series of
owner-directed design and implementation requests, covered below.

## 2. Outcome in short

- The #185 build was staged and verified exactly as briefed.
- Jack approved the label placement ("honestly looks great") and then asked for
  follow-ups: city dodging, city names, bold countries, a typography convention
  and wrapping.
- Those are implemented on one task branch as six commits and staged as builds.
- **Jack approved both of the latest builds** (`e7bec14` and `da2c148`) on the G4.
  That approval is the owner's statement in chat. No acceptance checkbox has
  been ticked, and no timing numbers have been given to me.

## 3. Timeline

1. **#185 staging.** Revision `b7aa82a587dbc3a38c1ed8af3411b0d516db068e`.
   `PoodleEarth-pr185-b7aa82a-timing.bin` is 592,512 bytes, SHA-256
   `d5f9a7a48c41809406dceec1ab795190d6bf604e3faec6d6c26c0647fd652d3c`.
   I uploaded it in binary mode under a distinct name and verified it by
   downloading it back.
2. **Folder tidy.** At Jack's request: moved it into `PoodleEarth/`, put the
   existing contents under `v0.1.0/`, and made `v0.1.1/` for the new file.
3. **E1/E2.** Jack asked who they were. They are two evaluation items from #185's
   test plan (see that issue for the exact wording). Jack passed E1, declined to test font removal,
   and called E2 "good for now". I posted that as an evidence comment, with
   Jack's agreement.
4. **City dodging.** Jack asked for a follow-up issue for 0.1.3, which became #217.
5. **Fonts.** Jack wanted Palatino for countries, then proposed the rule
   "Palatino for geographic features, Helvetica for political ones". He later
   reported that his screen showed only Helvetica on the new build, although
   Palatino is installed. That is still open as a diagnosis.
6. **City names (#219).** Jack asked for city labels chosen by population tier
   and zoom, with distance buffering. Jack gave "go ahead with your
   recommendation", and I implemented it (§4).
7. **Country names duck city dots, and bold countries.** Jack asked for this.
   He floated italic instead, so I discussed it, and the italic idea went into
   the convention (reserved for water).
8. **Typography convention v1.** Written into `docs/design/typography.md`
   under #219 with Jack's "take all your recommendations".
9. **Convention applied to labels.** Helvetica with a Geneva fallback; 10 pt
   for large cities and 9 pt for the rest.
10. **Wrapping.** Max width, at most three lines, centred.
11. **Architecture check.** Jack asked twice whether this respects the Elm
    architecture. I answered, and one gap is open (§7).

## 4. What was built (branch `claude/city-labels-219-20261119`)

| Commit | Content |
|---|---|
| `08fd622` | City names by population tier and zoom; GPLC v2 tier field |
| `7c1f44d` | Country names duck city dots and draw in bold (#219, #217) |
| `3cba649`, `8be2555` | `docs/design/typography.md`, convention v1 and delegated decisions |
| `e7bec14` | Convention applied: Geneva fallback, 10 pt / 9 pt |
| `da2c148` | Wrap long country and city names |

Design points:

- **Placer.** `place_labels` is pure and portable. Labels carry a side
  (`center` or `right`), a max width and a priority. It takes an optional
  `avoid` span. Labels beside a dot avoid hard, and centred labels duck up,
  down, right then left. Caps: 256 candidates and 32 placed labels.
- **Data.** The city pack moved to GPLC v2. The last uint32 holds a population
  tier from 0 to 3, with floors at 10M, 5M and 2.5M. The pack is 9,641 bytes,
  SHA-256 `2cd7ce0828794e858e941b526ceabb30333628579cec857592f9bfdab7ee322d`,
  with tier counts 27, 43, 43 and 191. The reader accepts v1 and v2, and v1
  reports tier 3. The licence and source docs were updated.
- **Zoom.** `max_place_tier` takes the camera radius over the shorter side.
  Under 0.8 gives tier 0, under 1.4 tier 1, under 2.6 tier 2, else tier 3.
- **Wrapping.** Greedy at spaces; widths 80 px (countries) and 64 px
  (cities); at most three lines. A word longer than the width stays whole.
  Countries centre each line, and cities align left beside the dot.
- **Menu.** A "City Names" toggle was added to View on OS 9 and to the X11
  tools window.
- **Memory.** One shared heap array of 96 placed labels (3 × 32) serves a frame
  on each front end, so no large stack arrays are needed on Classic.

Elm architecture: the placer, selection and tier logic are pure and sit in the
portable core. The name toggles are currently front-end state, not part of
Model/Msg (§7).

## 5. Staged builds on the FTP share

All under `ftp://192.168.1.62/PoodleEarth/v0.1.1/`, uploaded in binary mode.
All are PowerPC timing builds (`-DGLOBE_TIMING_TITLE=ON`, Retro68). The title
bar adds `cities N`.

- `PoodleEarth-pr185-b7aa82a-timing.bin`: 592,512 bytes (the original brief).
  The folder tidy placed it in `v0.1.1/`.
- `PoodleEarth-city-labels-219-da2c148-wrap-timing.bin`: 596,608 bytes,
  SHA-256 `6f513439673cd7f0a51548145ed3fbbd00e39910a95591a02e26c48edbb5026c`,
  download-back verified.
- A build for `e7bec14` (no wrapping) is in the same folder, also staged and
  approved. Its checksum is not repeated here; compute it from the share before
  relying on it.

## 6. Verification, and what was not done

Run on the laptop (Linux, GCC 16.2.1):

- `ctest` 69/69, including the new wrap, tier, avoid and builder tests.
- clang-format and clang-tidy 22.1.8 clean on the changed non-OS-9 sources.
- `tools/check-policy.sh` ok.
- A Retro68 PowerPC timing build packaged.
- An Xvfb screenshot of the X11 build, where "Democratic Republic of the
  Congo" wraps to three centred lines.

**Not run, or not verified:**

- Behaviour and timing on the G4. Jack said he approved the builds, but gave me
  no figures or observations, and I record none. Host tests and cross
  compilation are not hardware acceptance.
- The 68k build.
- clang-tidy on `src/platform/os9`.
- The extra Classic memory use. My estimate is about 26 KiB of candidate vectors
  plus the 96-entry array. It is unmeasured.
- Whether Palatino appears on the G4. Jack saw only Helvetica on one earlier
  build. Under the convention, Palatino is for physical features, and countries
  are political, so Helvetica is intended for countries. The diagnosis of the
  report is not closed. If Palatino is expected anywhere, it does not exist yet
  because no water or terrain labels are in the code.
- Not implemented: a capital marker (the city pack has no capital flag) and a
  heading font on the info card (the code has no font styling there).

## 7. Mistakes and open points

- **#218 was opened without being asked.** I opened it for the Palatino
  discussion, and Jack said "I'm exploring, I didn't know you would open an
  issue". I should have proposed it. It is still open, and I have asked about
  closing it as not planned. Please do not treat it as scope.
- I made one rejected tool call and carried on when Jack said "continue".
- **Elm gap.** The country and city toggle state lives in the front ends, not
  in the Model. This is the one point on which I did not fully meet the
  architecture rule. I proposed moving it into Model/Msg and have not been
  given an answer.

## 8. Issues and milestones now

| Issue | State | Milestone | Notes |
|---|---|---|---|
| #185 | open | v0.1.2 | Country names PR; the branch depends on its unmerged draft |
| #217 | open | v0.1.3 | Country labels dodge city dots; code is on the branch |
| #218 | open | v0.1.2 | Opened in error (§7); proposed close as not planned |
| #219 | open, `status: in progress` | v0.1.3 | City names; acceptance boxes unticked, G4 item is the owner's |
| #52, #53 | open | none | The milestone was asked for and not decided |

Milestones: v0.1.2 is current, v0.1.3 planned, v0.1 closing. The G4
acceptance and the final timing numbers are Jack's. I have not set
`status: ready` on anything or ticked any acceptance box.

## 9. Decisions waiting on Jack

1. Close #218 as not planned?
2. Which milestone for #52 and #53?
3. Open a PR for the branch (it depends on #185, so target its branch or wait)?
4. Move the name toggles into Model/Msg?
5. Does the capital marker justify a pack data change (flag in GPLC v3)?
6. Record the G4 observations on #219 once Jack has them.

## 10. For the next agent

Read AGENTS.md, then #219 with every comment, then
`docs/design/typography.md` and `docs/design/country-labels.md`. Do not touch
the G4 or the share without Jack's say-so. Keep the FTP login and Jack's email
out of every record.
