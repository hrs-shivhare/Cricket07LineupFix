# Validation — 22 October 2026

Build: `Cricket07Lineup.asi` 1.1, SHA-256 `4102498252937a2a5a0c1f8b4aa0b969838f989377f94cd06b4370d899ae79a5`.

All checks below passed. No `FALLBACK` or `NOT APPLIED` lines appeared in the game test log.

| Check | Coverage | Evidence limit |
|---|---|---|
| Portable core | All seven eligible batting roles in both formats; zero through three batting ALL/ASP credits against one/two-place requirements; original Test flex; 3,000 varied squads | Executes the replacement selection core, without game functions |
| Actual roster reference replay | 63 squads × 17 pitch IDs × 2 formats = 2,142 cases | Reads extracted CDF; specialist bowling stages are an explicitly labeled reference model |
| Native x86 adapter | 128 native-stage checks, both sides, formats, OWK/non-OWK, zero through three batting ALL/ASP, flags, reserve metadata, fallback and reentrancy | Calls executable fixture stubs rather than Cricket07 functions |
| Detour and DLL | Executed x86 detour/trampoline, refusal of conflicting signatures, DLL exports, unsupported executable refusal | Fixture executable for negative checks |
| Actual game startup | Ultimate ASI Loader loaded version 1.1; exported status = 1; live entry bytes contain the expected detour | Saved in `tests/runtime-startup-v1.1.json` |
| Actual game selection | 16 controlled calls through original wrapper `0x4180D0`, using installed game functions and current loaded roster | Saved in `tests/runtime-lineups-v1.1.json`; no full match played |
| Protected file integrity | EXE, loader DLL, existing FileLoader/CPU ASIs, data.gob, loose roster archive | All six hashes identical before and after the game test |

The native integration replay ran from a game thread at a window-message return. It changed transient pitch, overs and selected lineups in a freshly launched test process. It restored pitch/overs, detached instrumentation, and closed that test process. It did not write roster, executable or saved-game files. The installed ASI works independently of the instrumentation.

The 16 in-game cases:

| Squad | Pitch × format combinations | ALL/ASP in batting trio | Result |
|---|---|---:|---|
| Australia, ID 0 | Normal/Green × Limited/Test | 0 | All four passed |
| India, ID 13 | Normal/Green × Limited/Test | 0 | All four passed |
| Team ID 1 | Normal/Green × Limited/Test | 1 | All four passed |
| Team ID 15, OWK keeper K.Otto | Normal/Green × Limited/Test | 2 | All four passed; OWK at No. 1 |

Each runtime case checked keeper priority and Skill ranking, membership in the live squad, 11 unique XI players, the fixed opening positions, descending Skill among other players, and six distinct bowling-order players belonging to the XI, excluding the designated keeper. It also compared XI membership against the revised batting-trio, credit, additional-pick and Test-flex rules. The expected specialist pool uses a separately expressed reference model; the native internal pool was not directly captured. The actual selections still execute the original native bowling helpers inside the game.

Observed limited-overs example with one batting credit (team ID 1):

- The batting trio includes Alik Kapeli, role ASP.
- Normal: the single ALL/ASP requirement is already met; the additional Accuracy pick is Enamil Hake, role SBW.
- Green: one more ALL/ASP is required. Khalid Mamed, role ALL, wins that Accuracy pick; Al Sahara, role BAT, wins the second pick from any role by Skill.

This directly exercises the clarified rule: one credited batting all-rounder on Green leaves one ALL/ASP pick, followed by one unrestricted Skill pick.

Observed limited-overs example with two batting credits (team ID 15):

- S.Tirol (ALL) and M.Odenth (ASP) both qualify among the batting trio.
- On Green, T.Odenko (ALL) wins the unrestricted Accuracy pick, and B.Paters (BAT) wins the unrestricted Skill pick.
- The XI therefore includes at least three ALL/ASP players. The quota does not exclude an additional all-rounder who wins on merit.
- The OWK keeper K.Otto remains at No. 1 in every case.

These are observations for the loaded roster. Editing squad roles or ratings can change the outcome. Runtime coverage does not establish every career/tournament UI path or long-running match stability. The executable identity guard intentionally disables the plugin on other executable builds.

Reproduce the automated checks with `build.ps1`. The controlled Frida script `tests/runtime_smoke.js` requires a disposable running game process with the database loaded; attach it through REA's `instrument_with_frida` for about five seconds. It may change the displayed XI, so it should not run during an ongoing match. The JSON preserves complete player IDs, raw ratings, batting trio, credited count, additional picks and bowling order for all 16 observed cases. `tests/build-v1.1.txt` preserves the automated check output; `tests/runtime-game-v1.1.log` contains the version 1.1 game-log segment. Version 1.0 and its prior evidence remain under `releases/1.0`.
