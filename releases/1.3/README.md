# Cricket07Lineup 1.3

32-bit ASI plugin for automatic XI selection in the installed Windows Cricket 07.
Installed at `E:\Cricket07\Cricket07Lineup.asi`. Ultimate ASI Loader loads it on the next game launch.
It replaces the automatic lineup builder in process memory; the executable and roster files stay unchanged.
Manual changes to an already selected XI remain possible. Automatic selection runs again whenever the game invokes its lineup builder.

## Selection rules

1. Choose the keeper from the first nonempty priority tier: WK/WKB/OWK roles; primary wicketkeeping field position; secondary wicketkeeping field position; native legacy keeper field equal to 4; any player. Within that tier choose the highest batting Skill. Equal Skill retains the last squad entry, matching the original keeper tie behavior.
2. If that keeper is OWK, fix him at No. 1 and the best remaining OBT/OWK at No. 2. Otherwise choose the best two remaining OBT/OWK: highest Skill at No. 2, next at No. 1. Leave missing opener positions available for the later Skill sort.
3. Execute the game's original four specialist-bowler selection passes. Their pitch quotas, Accuracy comparisons and bowling-order placement are retained. Keeper and fixed openers are unavailable to those passes. Temporarily disable their two hardcoded player-ID exclusion switches.
4. In both limited overs and Tests, select up to three remaining players by batting Skill from BAT, ALL, ASP, OBT, WK, WKB and OWK. Previously selected players are unavailable, so neither a keeper nor an opener nor a specialist bowler can occupy a second XI place.
5. In limited overs, identify the ALL/ASP player who would win the next all-rounder pick by bowling Accuracy, after keeper/openers/specialist bowlers have been selected but before the batting trio. If that exact player is selected among the three batting picks, the Accuracy requirement is fulfilled: the first additional place accepts any role by batting Skill; the second, when the pitch allocates one, remains restricted to ALL/ASP by batting Skill. This second-place restriction applies even if the batting trio already includes multiple ALL/ASP. Otherwise retain the previous rule: count batting-trio ALL/ASP against the requirement; fill remaining required ALL/ASP first, then accept any role in freed places; the first additional pick uses Accuracy and the second uses Skill. There is no maximum on total ALL/ASP selected.
6. In Tests, retain the original flexible selection after the broader batting trio: on pitches outside the Damp/Green/Wet group, choose one best remaining BAT or ALL by Skill, preferring ALL on a BAT/ALL tie. On Damp/Green/Wet there is no extra flexible pick. ASP can now qualify among the first three batting picks, but is not added to the Test flex pool.
7. Fill any remaining XI shortages from any role by batting Skill.
8. In both formats, keep the established opening pair fixed. If more selected players have role OBT/OWK, place these extra openers at No. 3, No. 4, etc., by descending batting Skill, ahead of other roles even when those other players have higher Skill. Then order everyone else, including a non-OWK keeper, by Skill. Missing opening positions still receive the best available player through the shortage ordering. There is no keeper Skill-80 promotion or reserved No. 7 slot. This ordering does not select additional openers into the XI.
9. Complete the bowling order through the original native function. Retain the original captain selection and finalization. Sort unused squad members by Skill onto the bench.

“Batting accuracy” in the request is implemented as batting **Skill**; bowling **Accuracy** is a separate field. All comparisons use the stored six-bit ratings rather than rounded display values. Other equal-rating selections preserve squad order; the final Skill sort is stable. No player ID is excluded by the replacement selection rules.

Pitch identifiers used by the native game:

| Group | Internal IDs | Limited additional places | Test flexible BAT/ALL pick |
|---|---|---:|---:|
| Dusty | 1, 6, 7, 15 | 2 | 1 |
| Damp / Green / Wet | 4 / 8, 9 / 16 | 2 | 0 |
| Normal / Dry / Astro / Hard | 0, 12, 13 / 2, 5 / 3 / 10, 11, 14 | 1 | 1 |

The game's specialist-bowler passes run before the batting trio. ASP can already have been selected there and is unavailable for another XI place. Only ALL/ASP among the three new batting picks earn credits against the later requirement; players picked earlier as keeper, opener or specialist bowler do not earn these credits.

If the counterfactual Accuracy winner **is** in the batting trio:

| Pitch allocation | First additional pick | Second additional pick |
|---:|---|---|
| 1 | Any role by Skill | None allocated |
| 2 | Any role by Skill | ALL/ASP only by Skill |

Equal-Accuracy ties use the first squad entry, matching the actual selection comparator; selecting a later tied player does not earn this credit.

If the Accuracy winner is **not** in the batting trio:

| Pitch allocation | ALL/ASP in the batting trio | First additional pick: Accuracy | Second additional pick: Skill |
|---:|---:|---|---|
| 1 | 0 | ALL/ASP | None allocated |
| 1 | 1–3 | Any role | None allocated |
| 2 | 0 | ALL/ASP | ALL/ASP |
| 2 | 1 | ALL/ASP | Any role |
| 2 | 2–3 | Any role | Any role |

All picks are subject to remaining XI capacity. If a required ALL/ASP pool is empty, the unchanged general shortage fill completes the XI by batting Skill. It also fills other leftover places after the allocated picks. When Accuracy is already credited, the additional batting picks do not reserve bowling-order seats; the original native completion fills vacant bowling-order places from the selected XI.

Example from the captured 20-player South Africa squad: Kallis wins a batting place and is also the counterfactual ALL/ASP Accuracy winner (raw Accuracy 56, ahead of Hall at 54). On Normal limited overs, de Villiers wins the freed Skill pick and bats at No. 6. On Green, de Villiers wins the any-role Skill pick, then Hall wins the ALL/ASP-only Skill pick.

## Build and install

From the workspace root in PowerShell:

```powershell
& .\plugins\Cricket07Lineup\build.ps1
& .\plugins\Cricket07Lineup\build.ps1 -Install
```

The script uses the existing Zig 0.15.2 toolchain, builds an x86 DLL with an `.asi` extension, runs the core, captured live-squad, roster-reference and native-adapter tests, and optionally copies it to the game directory. `-Zig` and `-RosterPath` override the compiler and extracted test CDF paths. Close the game before installation. Recognized versions 1.0, 1.1 and 1.2 are backed up under their respective `releases/` folders before upgrading; an unrecognized installed ASI is not overwritten.

Supported executable SHA-256:

```text
d776dba0cc506b968f7d228e036cf42aeb4abe45a8d1ad0d7226cdebf91553a2
```

Built ASI SHA-256:

```text
d09ab092985187c6534486ec970cd6afc44d9bf643dae4f5a46b39daeccfb1cb
```

`Cricket07Lineup.log` beside the ASI reports `APPLIED` and each selected XI, with player IDs and displayed batting Skill. `NOT APPLIED` indicates an unsupported executable or conflicting hook. Unexpected selector/roster input produces a logged `FALLBACK` to the original builder for that request, including its original behavior.

To uninstall: close the game and remove only `E:\Cricket07\Cricket07Lineup.asi`. Its optional `.log` can also be removed. Keep `ddraw.dll` and the other ASIs.

## Implementation and evidence

- `src/selection.c`: portable selection and ordering core.
- `src/plugin.c`: native roster decoding, original bowling calls, finalization, logging and executable identity guard.
- `src/hook.c`: guarded eight-byte detour and fallback trampoline.
- `tests/`: core cases, actual-CDF reference replay, executable x86 adapter/hook fixtures, and controlled in-game integration evidence.
- [VALIDATION.md](VALIDATION.md): exact validation scope and observed examples.

The only entry detour is original VA `0x417330` / RVA `0x17330`. Displaced instructions are `sub esp,0x1c; mov eax,[0x815e2c]`, with the absolute operand checked against the loaded image address. The DLL is pinned for the process lifetime. Initialization occurs through the loader's `InitializeASI` export, outside `DllMain`.

Retained native functions: specialist bowling `0x4161F0`, `0x416370`, `0x416610`, `0x416890`; bowling completion `0x413400`; captain `0x4DA740`; finalization `0x414950`. Runtime tests invoke the original automatic-selection wrapper at `0x4180D0`.

Player records are resolved through `0x4D8F10` from the game's currently loaded squad, rather than a bundled copy of the roster. Record fields: role `+0x15F & 15`; batting Skill `(u32(+0x6C)>>6)&63`; bowling Accuracy `(u32(+0x74)>>12)&63`; field-position tiers from `+0x84`, and legacy keeper field `u32(+0x8C)&63`. Catching no longer ranks keeper candidates.
