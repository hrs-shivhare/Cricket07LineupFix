# Cricket07Lineup 1.0

32-bit ASI plugin for automatic XI selection in the installed Windows Cricket 07.
Installed at `E:\Cricket07\Cricket07Lineup.asi`. Ultimate ASI Loader loads it on the next game launch.
It replaces the automatic lineup builder in process memory; the executable and roster files stay unchanged.
Manual changes to an already selected XI remain possible. Automatic selection runs again whenever the game invokes its lineup builder.

## Selection rules

1. Choose the keeper from the first nonempty priority tier: WK/WKB/OWK roles; primary wicketkeeping field position; secondary wicketkeeping field position; native legacy keeper field equal to 4; any player. Within that tier choose the highest batting Skill. Equal Skill retains the last squad entry, matching the original keeper tie behavior.
2. If that keeper is OWK, fix him at No. 1 and the best remaining OBT/OWK at No. 2. Otherwise choose the best two remaining OBT/OWK: highest Skill at No. 2, next at No. 1. Leave missing opener positions available for the later Skill sort.
3. Execute the game's original four specialist-bowler selection passes. Their pitch quotas, Accuracy comparisons and bowling-order placement are retained. Keeper and fixed openers are unavailable to those passes. Temporarily disable their two hardcoded player-ID exclusion switches.
4. Select up to three remaining BAT players by batting Skill, subject to XI capacity.
5. In limited overs, choose from remaining ALL and ASP players. First pick: highest bowling Accuracy. Second pick, when allowed: highest batting Skill. Players already selected as specialist bowlers are unavailable. There are two all-rounder picks on Dusty, Damp, Green and Wet pitch variants; otherwise one. These are additional picks after the specialist group, subject to XI capacity and available candidates.
6. In Tests, retain the original flexible selection: on pitches outside the Damp/Green/Wet group, choose one best remaining BAT or ALL by Skill, preferring ALL on a BAT/ALL tie. On Damp/Green/Wet there is no extra flexible pick. ASP is not added to this Test pool.
7. Fill any remaining XI shortages from any role by batting Skill.
8. Keep only the established opener positions fixed. Order all other selected players, including a non-OWK keeper, by batting Skill. A missing opening position receives the best player in this sort. There is no keeper Skill-80 promotion or reserved No. 7 slot.
9. Complete the bowling order through the original native function. Retain the original captain selection and finalization. Sort unused squad members by Skill onto the bench.

“Batting accuracy” in the request is implemented as batting **Skill**; bowling **Accuracy** is a separate field. All comparisons use the stored six-bit ratings rather than rounded display values. Other equal-rating selections preserve squad order; the final Skill sort is stable. No player ID is excluded by the replacement selection rules.

Pitch identifiers used by the native game:

| Group | Internal IDs | Limited all-rounder picks | Test flexible BAT/ALL pick |
|---|---|---:|---:|
| Dusty | 1, 6, 7, 15 | 2 | 1 |
| Damp / Green / Wet | 4 / 8, 9 / 16 | 2 | 0 |
| Normal / Dry / Astro / Hard | 0, 12, 13 / 2, 5 / 3 / 10, 11, 14 | 1 | 1 |

The game's specialist-bowler passes run before these all-rounder picks. ASP can already have been selected there, and will never be counted twice.

## Build and install

From the workspace root in PowerShell:

```powershell
& .\plugins\Cricket07Lineup\build.ps1
& .\plugins\Cricket07Lineup\build.ps1 -Install
```

The script uses the existing Zig 0.15.2 toolchain, builds an x86 DLL with an `.asi` extension, runs the core, roster-reference and native-adapter tests, and optionally copies it to the game directory. `-Zig` and `-RosterPath` override the compiler and extracted test CDF paths. Close the game before installation. The installer refuses to replace a different existing `Cricket07Lineup.asi`.

Supported executable SHA-256:

```text
d776dba0cc506b968f7d228e036cf42aeb4abe45a8d1ad0d7226cdebf91553a2
```

Built ASI SHA-256:

```text
4f1568cc3b0f195103b3dcabf1efe18b2ba3ba5179f814c795d9096ab0360362
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
