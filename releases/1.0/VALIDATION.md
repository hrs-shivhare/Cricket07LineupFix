# Validation — 22 October 2026

Build: `Cricket07Lineup.asi` 1.0, SHA-256 `4f1568cc3b0f195103b3dcabf1efe18b2ba3ba5179f814c795d9096ab0360362`.

All checks below passed. No `FALLBACK` or `NOT APPLIED` lines appeared in the game test log.

| Check | Coverage | Evidence limit |
|---|---|---|
| Portable core | Directed role/keeper/opening/ALL cases plus 3,000 varied squads | Executes the replacement selection core, without game functions |
| Actual roster reference replay | 63 squads × 17 pitch IDs × 2 formats = 2,142 cases | Reads extracted CDF; specialist bowling stages are an explicitly labeled reference model |
| Native x86 adapter | 32 native-stage checks, both sides, formats, OWK/non-OWK, flags, reserve metadata, fallback and reentrancy | Calls executable fixture stubs rather than Cricket07 functions |
| Detour and DLL | Executed x86 detour/trampoline, refusal of conflicting signatures, DLL exports, unsupported executable refusal | Fixture executable for negative checks |
| Actual game startup | Ultimate ASI Loader loaded the new DLL; exported status = 1; live entry bytes contain the expected detour | Saved in `tests/runtime-startup.json` |
| Actual game selection | Nine controlled calls through original wrapper `0x4180D0`, using installed game functions and current loaded roster | Saved in `tests/runtime-lineups.json`; no full match played |
| Protected file integrity | EXE, loader DLL, existing FileLoader/CPU ASIs, data.gob, loose roster archive | All six hashes identical before and after the game test |

The native integration replay ran from a game thread at a window-message return. It changed transient pitch, overs and selected lineups in a freshly launched test process. It restored pitch/overs, detached instrumentation, and closed that test process. It did not write roster, executable or saved-game files. The installed ASI works independently of the instrumentation.

The nine in-game cases:

| Squad | Pitch | Format | Keeper | Position | Result |
|---|---|---|---|---:|---|
| Australia | Normal | Limited | A.Gilchrist | 6 | Passed |
| Australia | Green | Limited | A.Gilchrist | 7 | Passed |
| Australia | Normal | Test | A.Gilchrist | 7 | Passed |
| Australia | Green | Test | A.Gilchrist | 6 | Passed |
| India | Normal | Limited | D.Dhenier | 6 | Passed |
| India | Green | Limited | D.Dhenier | 6 | Passed |
| India | Normal | Test | D.Dhenier | 7 | Passed |
| India | Green | Test | D.Dhenier | 6 | Passed |
| Team ID 15 | Normal | Limited | K.Otto, role OWK | 1 | Passed |

Each runtime case checked keeper priority and Skill ranking, membership in the live squad, 11 unique XI players, the fixed opening positions, descending Skill among other players, and six distinct bowling-order players belonging to the XI, excluding the designated keeper.

Observed Australian limited-overs example:

- Normal: Langer, Hayden, Ponting, Martyn, Clarke, Gilchrist, Watson, Warne, Lee, Clark, McGrath.
- Green: Langer, Hayden, Ponting, Martyn, Clarke, Symonds, Gilchrist, Watson, Warne, Lee, McGrath.

Watson is the first all-rounder pick because of higher bowling Accuracy. On Green, Symonds is added by batting Skill and bats above Watson. Gilchrist's position follows the selected XI's Skill order. Langer/Hayden remain fixed at No. 1/No. 2.

Observed Indian limited-overs example:

- Normal: Gemphir, Seway, Tendehar, Dravia, Gungly, Dhenier, Yevray Singh, Hurvaj Singh, Z.Kalm, Agercker, Naharem.
- Green: Gemphir, Seway, Tendehar, Dravia, Gungly, Dhenier, Yevray Singh, Bengel, Hurvaj Singh, Z.Kalm, Agercker.

These are observations for the loaded roster. Editing squad roles or ratings can change the outcome. Runtime coverage does not establish every career/tournament UI path or long-running match stability. The executable identity guard intentionally disables the plugin on other executable builds.

Reproduce the automated checks with `build.ps1`. The controlled Frida script `tests/runtime_smoke.js` requires a disposable running game process with the database loaded; attach it through REA's `instrument_with_frida` for about five seconds. It may change the displayed XI, so it should not run during an ongoing match. The JSON preserves complete player IDs, raw ratings and bowling order for the nine observed cases.
