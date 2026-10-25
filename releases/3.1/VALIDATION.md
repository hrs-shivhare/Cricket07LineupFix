# Validation - 24 October 2026

Build: Cricket07Lineup 3.1. SHA-256: 1a0a2bebbd022f4846346301d6bcb8c3a8b0419381a3bd516ff265f47796aa5f.

| Check | Coverage | Result |
|---|---|---|
| Directed core | Early Test ALL/BAT flex; Skill ranking/ties; exclusion of ASP/WK/OBT from that early pool; different later bowling pick; no-flex Green pitches; missing flex carried to final batting; prior keeper/openers/fallback rules | Pass |
| Varied squads | 6,000 inputs with 11-20 players; uniqueness, keeper, seeds, bench and order; invalid-input output remains unchanged | Pass |
| Frozen live South Africa fixture | Captured 20-player roster; unchanged limited XI and changed Normal Test XI/seed | Pass |
| Extracted roster replay | 63 squads x 17 pitches x 2 formats = 2,142 inputs | Pass |
| v3.0 comparison | Both cores on identical roster inputs; exact unchanged-output assertions for limited overs and Green/Damp/Wet Tests | Pass |
| x86 native adapter and hook | 136 side/format/keeper/pitch cases; production adapter ABI, native-completion stubs, detour/trampoline, exports and wrong-host refusal | Pass |
| Installed binary | Installed ASI hash equals tested dist artifact; supported EXE hash checked by installer | Pass |
| Protected files | EXE, data.gob, ddraw.dll and five other installed ASIs | All 8 unchanged |

No new in-game replay was performed for v3.1. The 36 actual-game cases documented in releases/3.0/VALIDATION.md apply to v3.0. Native adapter tests for this release use fixture stubs for original completion/captain/finalization, so they do not establish live gameplay or long-run match stability. The updated tests/runtime_smoke.js is available for a later controlled game replay.

## Comparison results

| Format | XI membership changed | Order-only changed | Keeper changed | Bowling seed changed |
|---|---:|---:|---:|---:|
| Limited | 0/1,071 | 0 | 0 | 0 |
| Test | 71/1,071 | 0 | 0 | 102 |

All Green/Damp/Wet Test results remain identical. Only the 13 pitch IDs with a Test flex can differ. Seed counts refer to the partial bowling order, before native completion; a flex player can still be added by that native function.

South Africa Normal/Dusty Test: Kallis now occupies the early batting flex; Hall is picked by a later bowling stage and replaces Bosman in the XI. Australia and India Normal/Green/Dusty examples retain the same XI. These are source replays against the extracted roster, not observations from a running v3.1 game.

If no ALL/BAT exists for the early Test flex, the implementation leaves it vacant and adds one to the final broad batting count, before any-role Skill fallback. Equal Skill in that early pool uses squad order. This resolves the unspecified empty-pool case consistently with deferred opener vacancies.

## Reproducibility and evidence

- build.ps1: compilation, all automated checks, supported-executable/upgrade guards and installation.
- compare-v3.0.ps1: preserved v3.0 core linked beside v3.1 with identical inputs.
- tests/build-v3.1.txt: build/test/install results.
- tests/roster-compare-v3.1.txt: full comparison and named deltas.
- tests/protected-files-v3.1-before.json and protected-files-v3.1-after.json: matching installation fingerprints.
- releases/3.0/: previous ASI, complete sources, tests and documentation.

Only Cricket07Lineup.asi was replaced in the game directory. No EXE, database or unrelated plugin was edited. No Frida/REA runtime dependency is introduced.