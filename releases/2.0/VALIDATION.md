# Validation — 23 October 2026

Build: Cricket07Lineup 2.0. SHA-256: bacba6c6ed903fe4cd19815a2f7674d3a311ea1ba9c97c6dc90051b8f01ae26f.

All checks below passed. The installed build matches the tested build.

| Check | Coverage | Scope |
|---|---|---|
| Directed portable-core tests | Keeper tiers, all seven batting roles, batting before bowling, zero/one/two missing openers, opening-role keeper, every bowling fallback, BWL/ALL Accuracy competition, both ALL picks by Accuracy, Test flex tie, all-opener Skill sort, deferred shortages and invalid inputs | Replacement core only |
| Varied squads | 6,000 squads with 11–20 players and varied roles/Skill/Accuracy | XI/bench uniqueness, keeper inclusion, bowling-seed validity, final ordering and failure behavior |
| Captured South Africa fixture | Actual read-only 20-player snapshot; Normal/Dry/Hard/Green limited plus Normal Test | Replays new core; verifies changed membership and ordering |
| Extracted roster replay | 63 teams × 17 pitch IDs × 2 formats = 2,142 cases | Actual extracted CDF and replacement core; no modeled old bowling passes |
| Native x86 adapter | 136 cases: both sides × OWK/non-OWK keeper × both formats × 17 pitches | Production adapter with executable x86 fixture stubs |
| Retired selection helpers | Fixture stubs abort if any former specialist-role helper is called | All 136 adapter cases completed without calling them |
| Detour/DLL | Executed x86 detour/trampoline, conflict refusal, exported initialization/status and unsupported-host refusal | Fixture tests plus live loaded module |
| Actual game integration | 36 cases: six squads × Normal/Green/Dusty × Limited/Test | Original wrapper 0x4180D0 invokes installed 2.0 ASI, native bowling completion, captain and finalizer |
| Protected files | EXE, loader DLL, existing FileLoader/CPU ASIs, data.gob, loose roster archive | Matching before/after SHA-256 |

## Actual game observations

The 36 canonical cases used squad IDs 0, 13, 1, 15, 30 and 2 in a 20-player selection context. An independently expressed JavaScript model calculated expected stage picks from the actual loaded player records. The game's resulting XI matched its complete membership and order in every case.

Each case also checked the designated keeper, all XI IDs unique, sorted bench, reserve last-index metadata, exact preservation of the selected bowling seed, six distinct bowling-order players belonging to the XI, keeper excluded from bowling order, and captain belonging to the XI.

The saved per-stage trace is the independent expected calculation, validated against the actual resulting XI and bowling seed. It is not a per-instruction recording of the C core. Version 2.0 no longer invokes the old native role-selection helpers.

Observed South Africa limited XI on Normal, Green and Dusty:

1. G.Smith
2. B.Dippenaar
3. J.Kallis
4. H.Gibbs
5. J.Rudolph
6. M.Boucher
7. A.Hall
8. S.Pollock
9. N.Boje
10. J.Kemp
11. M.Ntini

The first six selected are the keeper Boucher, openers Smith/Dippenaar, and batting picks Kallis/Gibbs/Rudolph. On Normal, Pollock/Ntini fill OBW; Hall wins the additional BWL/ALL seat; Boje wins spin; Kemp wins ALL/ASP. On Green/Dusty, Hall and Kemp instead fill the two ALL/ASP places, both by Accuracy.

De Villiers is therefore omitted in limited overs by the new six-batting-first rule. On Normal and Dusty Tests, he wins the BAT/ALL flex pick and bats No. 6. Green Tests have no flex place, so he misses that XI.

All selected openers now lead by Skill. Smith is No. 1, Dippenaar No. 2. The old highest-opener-at-No.2 and OWK-fixed-No.1 rules have been removed.

## Files and reproduction

- Automated results: tests/build-v2.0.txt.
- Captured South Africa input: tests/south-africa-live-20261023.json.
- Live loaded module/status/hook: tests/runtime-startup-v2.0.json.
- Canonical 36-case output, stage traces and full player ratings: tests/runtime-lineups-v2.0.json.
- Game-log segment: tests/runtime-game-v2.0.log.
- Protected file digests: tests/protected-files-before.json and tests/protected-files-after.json.
- Earlier releases, sources and tests: releases/1.0 through releases/1.3.

Run build.ps1 to reproduce automated tests. For controlled integration, use REA instrument_with_frida with tests/runtime_smoke.js against a disposable Cricket07 process with the 20-player database context loaded. The script waits for a game message-return callback, invokes the original wrapper and modifies transient selection/pitch/overs state. Pitch and overs are restored, instrumentation detaches, and the test process used here was closed afterward. No roster/executable file writes occur. The installed ASI needs neither REA nor Frida.

The first startup status snapshot may show the initial 15-player context before the normal frontend initializes the 20-player context used by the canonical replay.

Coverage does not establish every career/tournament screen or long-running match stability. Unexpected roster/selector inputs log a FALLBACK to the original builder. Other executable builds and conflicting entry hooks are refused.
