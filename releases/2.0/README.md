# Cricket07Lineup 2.0

Installed at `E:\Cricket07\Cricket07Lineup.asi`. Ultimate ASI Loader loads it on the next game launch. The executable and roster files remain unchanged. This release replaces the earlier 1.x selection sequence.

## Selection sequence

Every pick excludes players already selected. Keeper and opening candidates are protected before the bowling passes.

1. Choose the highest-batting-Skill keeper from the first nonempty tier: WK/WKB/OWK roles; primary wicketkeeping field position; secondary wicketkeeping field position; native legacy keeper field equal to 4; any role. Equal Skill retains the last squad entry for this keeper choice.
2. Select the two best OBT/OWK by Skill. A designated keeper with either OBT or OWK role counts as one of the two, so only one other opening candidate is selected. Missing opener places remain available.
3. Select the best remaining OBT/WK/WKB/OWK/BAT/ALL/ASP by Skill until six players have been selected in total. This normally adds three; one missing opener adds four, and two missing openers add five. An opening-role keeper also normally requires four further batting picks. If the eligible pool runs out, leave the vacancies until later.
4. Fill two opening-bowler seats by Accuracy, preferring OBW first, then BWL for missing OBW seats, then any role for remaining vacancies.
5. Fill the pitch-dependent additional BWL seats from BWL/ALL by Accuracy. If unavailable, use any role by Accuracy.
6. Fill the pitch-dependent spin seats from SBW/ASP by Accuracy. If unavailable, use any role by Accuracy.
7. In limited overs, fill one or two additional ALL/ASP seats, both by Accuracy. If unavailable, use any role by Accuracy. Previously selected batting ALL/ASP do not cancel these seats; the former Accuracy-credit rules are removed.
8. In Tests, use the existing pitch-dependent flex BAT/ALL seat instead, ranked by batting Skill; an ALL wins a BAT/ALL Skill tie. There is no flex seat on Damp/Green/Wet.
9. If batting/flex shortages still leave the XI short, fill the final vacancies from any role by batting Skill.
10. Order every selected OBT/OWK at the top by descending batting Skill, then every other player by descending Skill. No opening position is fixed after selection. The best opener now bats No. 1; an OWK keeper is not automatically No. 1. The keeper has no reserved No. 7 position.

Except for the keeper tie and Test flex tie described above, equal ratings retain squad order. Final batting and bench sorts are stable. Skill and Accuracy comparisons use the game's stored six-bit ratings, rather than rounded display values.

## Pitch quotas

| Pitch group | Internal IDs | Format | OBW seats | Additional BWL/ALL | SBW/ASP | Additional ALL/ASP | Flex BAT/ALL |
|---|---|---|---:|---:|---:|---:|---:|
| Normal/Dry/Astro/Hard | 0,2,3,5,10,11,12,13,14 | Limited | 2 | 1 | 1 | 1 | 0 |
| Dusty | 1,6,7,15 | Limited | 2 | 0 | 1 | 2 | 0 |
| Damp/Green/Wet | 4,8,9,16 | Limited | 2 | 0 | 1 | 2 | 0 |
| Normal/Dry/Astro/Hard | 0,2,3,5,10,11,12,13,14 | Test | 2 | 1 | 1 | 0 | 1 |
| Dusty | 1,6,7,15 | Test | 2 | 0 | 2 | 0 | 1 |
| Damp/Green/Wet | 4,8,9,16 | Test | 2 | 2 | 1 | 0 | 0 |

With six initial batting selections, each row totals eleven. Pool shortages use the fallbacks at that exact stage. BWL filling a missing OBW seat does not add an extra BWL seat later. All earlier batting selections remain unavailable to the bowling selection pools.

Bowling seats seed the separate bowling order. The original native completion fills it to six XI members, excluding the designated keeper. Captain selection, reserve metadata and native finalization are retained. The selection core never applies hardcoded player-ID exclusions.

## Consequence for the captured South Africa squad

The first six are Boucher, Smith, Dippenaar, Kallis, Gibbs and Rudolph. On Normal limited overs, the remaining seats go to Pollock/Ntini (OBW), Hall (additional BWL/ALL), Boje (spin), and Kemp (ALL/ASP), using Accuracy.

De Villiers therefore misses this limited-overs XI under the new sequence. On a Normal Test pitch he wins the Skill-based flex BAT/ALL seat. This differs from version 1.3 because batting selection now ends at six before every bowling group is chosen.

## Build, install and remove

From the workspace root in PowerShell:

```powershell
& .\plugins\Cricket07Lineup\build.ps1
& .\plugins\Cricket07Lineup\build.ps1 -Install
```

The script uses the existing Zig 0.15.2 x86 toolchain, runs all automated checks and optionally installs the ASI. Close Cricket 07 before installation. `-Zig` and `-RosterPath` override the compiler and extracted test CDF paths. Recognized versions 1.0–1.3 are preserved under their `releases/` directories before replacement; an unrecognized installed ASI is not overwritten.

Supported executable SHA-256: `d776dba0cc506b968f7d228e036cf42aeb4abe45a8d1ad0d7226cdebf91553a2`.

ASI SHA-256: `bacba6c6ed903fe4cd19815a2f7674d3a311ea1ba9c97c6dc90051b8f01ae26f`.

`Cricket07Lineup.log` reports the release, installation status and automatic XI selections. Unexpected selector/roster inputs produce a logged `FALLBACK` to the original game builder for that request. Unsupported executables and conflicting entry hooks are refused.

Uninstall by closing the game and removing only `E:\Cricket07\Cricket07Lineup.asi`. Its log may also be removed. Keep `ddraw.dll` and the other installed ASIs.

## Source and verification

- `src/selection.c`: complete portable batting-first selection core, `c07_select`.
- `src/plugin.c`: live roster decoding, writing completed XI, native completion/captain/finalizer calls, logging and executable identity checks.
- `src/hook.c`: guarded eight-byte detour and fallback trampoline.
- `tests/`: directed tests, 6,000 varied squads, extracted-roster replay, captured live South Africa fixture, executable x86 adapter checks and controlled game integration.

The only detour is VA `0x417330` / RVA `0x17330`. The former role-selection helpers `0x4161F0/0x416370/0x416610/0x416890` are no longer called. Their pitch quotas are implemented in the replacement core with the requested new pools and fallbacks.

Retained native calls are bowling-order completion `0x413400`, captain `0x4DA740` and finalization `0x414950`. Game integration uses the original wrapper `0x4180D0`.

Player lookup remains `0x4D8F10`, using the currently loaded squad. Record fields: role `+0x15F & 15`; Skill `(u32(+0x6C)>>6)&63`; Accuracy `(u32(+0x74)>>12)&63`; keeper fallback field positions in `+0x84`, legacy field `u32(+0x8C)&63`. See [VALIDATION.md](VALIDATION.md) for evidence and limits.
