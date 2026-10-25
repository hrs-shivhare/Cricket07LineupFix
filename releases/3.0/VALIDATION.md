# Validation — 24 October 2026

Build: Cricket07Lineup 3.0. SHA-256: 1ca980c7ac867461eb128799f46772e680e15d8af0ebc52004d9c331c6acece4.

All automated checks and the canonical 36 in-game cases passed.

| Check | Coverage | Scope |
|---|---|---|
| Directed core tests | ALL quota first, fallback quota provenance, deferred OBW seats using BWL/ALL, spinner consumed by ALL quota, keeper selected from remainder, OWK keeper competition, 0/1/2 missing openers, broad final/Test flex pool, any-role Skill fallback, all-opener final order | Replacement core |
| Varied squads | 6,000 squads with 11–20 players, varied roles/ratings | Unique XI/bench, keeper inclusion/exclusion from bowling, ordering, valid seeds, failure behavior |
| Captured South Africa fixture | Limited Normal/Dry/Hard/Green and Normal Test using captured 20-player input | New membership and bowling seed verified |
| Extracted roster replay | 63 squads × 17 pitches × 2 formats = 2,142 cases | Actual extracted CDF and new core |
| Historical comparison | Same 2,142 inputs through saved v2.2 and current v3.0 | Counts membership/order/keeper/seed changes; prints named examples |
| x86 native adapter | 136 cases: sides × OWK/non-OWK × formats × 17 pitches | Production adapter with executable fixture stubs; original completion/captain/finalizer ABI |
| Hook/DLL | Executed x86 detour/trampoline, conflicting-entry refusal, exports and unsupported-host refusal | Fixture tests plus live module |
| Actual game | 36 cases: squads 0/13/1/15/30/2 × Normal/Green/Dusty × Limited/Test | Installed v3.0 called through original wrapper 0x4180D0 |
| Protected files | Supported EXE, data.gob, loader DLL and all other ASIs currently installed | Matching before/after hashes |

The old loose archive path used in earlier runs is no longer present in this installation. It was not recreated or modified. The current-file baseline tracks the eight files actually present: EXE, data.gob, ddraw.dll and the five other ASIs (FileLoader, CPU, BowlingPlan, StrokeVariation and WidescreenFix). Only Cricket07Lineup.asi was replaced.

## Comparison against v2.2

| Format | Membership changed | Order-only changed | Keeper changed | Bowling seed changed |
|---|---:|---:|---:|---:|
| Limited | 319/1,071 | 0 | 17 | 767 |
| Test | 415/1,071 | 0 | 0 | 320 |

Keeper and seed counts can overlap membership changes. The seed is the partially populated bowling order before native completion; it is not the full six-player native output. These statistics describe the extracted roster snapshot, not all possible edited rosters.

Reproduce with compare-v2.2.ps1. The saved old c07_select is compiled as c07_select_v22 and linked beside current c07_select. Both use identical CDF inputs. Full output: tests/roster-compare-v3.0.txt.

## Live game verification

The independent JavaScript model reads the actual loaded squad and calculates stage choices. Every canonical game call matched its complete XI membership/order and bowling seed. Checks also covered keeper, unique XI IDs, sorted bench, reserve last-index metadata, six distinct XI bowling-order players excluding the keeper, and captain belonging to the XI.

The trace is the independently calculated expected selection, matched against actual results. It is not a per-instruction trace of the C implementation.

Verified examples:

- Australia Normal limited: Watson is selected as the ALL quota first. Clark then wins the additional BWL/ALL place; Symonds loses his former XI place.
- South Africa limited Normal/Green/Dusty: Kallis is picked through ALL quota. The final three batters are Gibbs, Rudolph and de Villiers. Membership matches v2.2; the reason and bowling seed change.
- South Africa Normal/Dusty Test: Kallis is picked by bowling Accuracy before batting. The combined four-player final batting pool selects Gibbs, Rudolph, de Villiers and Bosman. Hall is omitted.
- South Africa Green Test: the two additional BWL/ALL picks are Kallis and Hall, freeing the final three batting picks for Gibbs, Rudolph and de Villiers. De Villiers replaces Kemp.
- India: all tested Normal/Green/Dusty XIs match v2.2.

The final order follows the user's explicit choice: every selected OBT/OWK is sorted together by Skill. The pair chosen during the opener step is not position-locked. An opening-role player consumed earlier by a bowling fallback is still part of the final opener group.

## Evidence and limits

- tests/build-v3.0.txt: automated test output.
- tests/roster-compare-v3.0.txt: full old/new roster comparison.
- tests/runtime-startup-v3.0.json: live module/status/hook.
- tests/runtime-lineups-v3.0.json: all 36 canonical cases, stage traces, raw ratings and bowling order.
- tests/runtime-game-v3.0.log: release-specific game log.
- tests/protected-files-before.json and protected-files-after.json: current installation fingerprints.
- releases/2.2/: previous installed ASI, sources, docs and tests; earlier releases also retained.

The controlled replay uses a disposable game process in its 20-player context, executes on a game message-return thread, and invokes the existing wrapper. It changes transient pitch/overs/selection state, restores pitch and overs, detaches instrumentation and closes that test process afterward. No executable/roster file writes occur. The installed ASI needs neither Frida nor REA.

These checks do not establish every career/tournament path or long-running match stability. Unknown executable builds and conflicting hooks are refused. Unexpected roster/selector inputs log FALLBACK and use the original builder for that request.
