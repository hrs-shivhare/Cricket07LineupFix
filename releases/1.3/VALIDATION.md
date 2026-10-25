# Validation — 23 October 2026

Build: `Cricket07Lineup.asi` 1.3, SHA-256 `d09ab092985187c6534486ec970cd6afc44d9bf643dae4f5a46b39daeccfb1cb`.

All checks below passed. No `FALLBACK` or `NOT APPLIED` lines appeared in the version 1.3 game-log segment.

| Check | Coverage | Evidence limit |
|---|---|---|
| Portable core | Accuracy-winner batting credit; first additional pick any role by Skill, second ALL/ASP only by Skill; higher-Skill BAT excluded from the restricted second pick; both ALL/ASP winners; first-in-squad Accuracy ties; extra opener ordering; 3,000 varied squads | Executes replacement selection core without game functions |
| Captured live South Africa squad | Exact 20-player read-only snapshot, Normal/Dry/Hard limited branches: de Villiers selected at No. 6 instead of Hall | Portable-core replay with the branch's specialist pool supplied |
| Actual roster reference replay | 63 squads × 17 pitches × 2 formats = 2,142 cases | Extracted CDF and explicitly modeled specialist-bowler stages |
| Native x86 adapter | 256 native-stage checks across sides, formats, OWK/non-OWK, batting credits and positive/negative Accuracy-winner credit; flags, reserve metadata, fallback and reentrancy | Executable fixture stubs rather than the game helpers |
| Detour and DLL | Executed x86 detour/trampoline; conflicting signatures refused; exports and unsupported-host refusal | Fixture executable for negative checks |
| Actual game startup | Version 1.3 loaded by Ultimate ASI Loader; exported status = 1; correct live entry detour | `tests/runtime-startup-v1.3.json` |
| Actual game selection | 24 controlled calls through original wrapper 0x4180D0; six squads × Normal/Green × Limited/Test, fully initialized 20-player context | `tests/runtime-lineups-v1.3.json`; no full match played |
| Protected file integrity | EXE, loader DLL, existing FileLoader/CPU ASIs, data.gob and loose roster archive unchanged | Matching before/after SHA-256 files |

The 24 canonical in-game cases used squads 0, 13, 1, 15, 30 and 2. Every case verified XI membership against the revised selection rules, unique XI members, keeper priority, fixed opening pair, extra-opener ordering, within-group Skill ordering, and six distinct bowling-order players in the XI excluding the keeper.

The expected specialist pool is a separate reference model, not a directly captured native pool. Actual selection still executes the game's original bowling helpers. The canonical replay waits for a 20-player selection context to match the user's captured South Africa case. The game log also includes normal startup selections outside the canonical replay.

## South Africa: verified changed behavior

In the user's captured squad, Kallis has raw bowling Accuracy 56, ahead of Hall at 54. Kallis wins one of the three batting places and also matches the ALL/ASP Accuracy winner determined before the batting trio. The Accuracy requirement is therefore credited.

Observed Normal limited XI:

1. B.Dippenaar
2. G.Smith
3. J.Kallis
4. H.Gibbs
5. J.Rudolph
6. A.de Villiers
7. M.Boucher
8. S.Pollock
9. N.Boje
10. M.Ntini
11. A.Nel

The additional pick is de Villiers by batting Skill, from any role. Hall is no longer selected merely to repeat an Accuracy requirement already met by Kallis.

Observed Green limited XI: Dippenaar, Smith, Kallis, Gibbs, Rudolph, de Villiers, Boucher, Hall, Pollock, Boje, Ntini. The first additional pick is de Villiers, any role by batting Skill. The second is Hall, ALL/ASP only by batting Skill. Bosman has higher batting Skill than Hall but cannot take the restricted second place. The same restriction applies when multiple ALL/ASP have already qualified through the batting trio.

The Test flex rule remains unchanged. The limited-overs Accuracy credit does not apply in Tests.

## Extra selected openers: verified changed batting order

Squad ID 2 contains a third selected opener, B.de Witmore. In all four tested pitch/format combinations:

- The fixed No. 1/No. 2 pair stays C.Summersfield / M.Brainst.
- B.de Witmore, Skill 62, bats at No. 3.
- M.Butcher, Skill 77, follows at No. 4 despite his higher Skill, because he is not an OBT/OWK.
- Other non-opener players follow by batting Skill.

This squad is identified by its database ID; it is not the England national team. Directed core tests additionally cover multiple extra OBT/OWK players at No. 3 and No. 4, including an extra OWK, and preserve the higher-Skill-first ordering among extra openers.

## Reproduction and limits

Run `build.ps1` for core, live-squad, full-roster and native-adapter checks. `tests/build-v1.3.txt` preserves their output. The regression input is retained in `tests/south-africa-live-20261023.json`.

For controlled game verification, attach `tests/runtime_smoke.js` with REA `instrument_with_frida` for roughly six seconds after the installed database has initialized. It invokes the normal wrapper on a game thread and changes transient lineup/pitch/overs state; use a disposable game process, not an ongoing match. Pitch and overs are restored before detachment. The test process used here was closed afterward. The installed ASI works without REA or Frida.

`tests/runtime-lineups-v1.3.json` preserves full player IDs/ratings, the counterfactual Accuracy winner, earned credit, additional-pick criteria, extra openers and bowling order. `tests/runtime-game-v1.3.log` preserves the game-log segment. Prior releases and evidence remain under `releases/1.0`, `releases/1.1` and `releases/1.2`.

Coverage does not establish every career/tournament UI path or long-running match stability. Other executable builds are disabled by the identity guard.
