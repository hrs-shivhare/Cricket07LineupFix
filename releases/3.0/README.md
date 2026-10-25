# Cricket07Lineup 3.0

Installed at E:\Cricket07\Cricket07Lineup.asi. Ultimate ASI Loader loads it on the next launch. This release replaces v2.2's batting-first selection with ALL-quota/bowling-first selection. Executable and roster files remain unchanged.

## Selection order

Every new pick excludes all players already selected. A player's selection quota is determined by the step that picked him, not by his nominal roster role.

1. Limited overs only: select the pitch's one/two ALL/ASP quota players by bowling Accuracy. When the role pool is empty, fill its remaining seats from any role by Accuracy. An OBW, WK or other role picked by this fallback still occupies an ALL-quota seat and is unavailable to later steps. Tests have no ALL quota here.
2. Select up to two remaining OBW by Accuracy. Do not fill missing OBW seats yet.
3. Add missing OBW seats to the pitch/format's additional-bowler quota. Fill this combined count from remaining BWL/ALL by Accuracy, then any role by Accuracy. The first replacements complete the opening-bowling pair; later picks fill additional-bowler places.
4. Select the pitch/format's one/two remaining SBW/ASP by Accuracy. When unavailable, use any role by Accuracy.
5. Choose the best batting keeper from the remaining squad using the first nonempty tier: WK/WKB/OWK; primary wicketkeeping field position; secondary wicketkeeping field position; legacy keeper field equal to 4; any role. Highest Skill wins within the tier; equal Skill retains the last squad entry.
6. Rank remaining OBT/OWK plus the selected keeper, if his role is OBT/OWK, by batting Skill. Choose up to two. An opening-role keeper is not forced into the pair if two other candidates have higher Skill. Missing opening-candidate places carry into the batting count.
7. Select the final batters by Skill from OBT/WK/WKB/OWK/BAT/ALL/ASP. Count = 3 + 1 if the chosen opening pair includes the keeper + missing opener places + the format/pitch's Test flex place. When that pool runs out, fill the XI from any remaining role by Skill, including bowling roles.

The Test flex place now belongs to this broad final batting pool. It is not a separate BAT/ALL-only pick and has no former ALL-over-BAT tie preference.

Final batting order remains as the user confirmed: **all selected OBT/OWK first by descending Skill, then everyone else by descending Skill**. The pair chosen in step 6 is not locked at No. 1/2. A player selected through a bowling fallback who has role OBT/OWK joins this same opener ordering. Equal ratings retain squad order, except the keeper tier tie noted above.

The v2.2 ALL/ASP credit mechanism is removed. ALL-quota players are selected first; no already-selected check converts those seats into batting-Skill picks.

## Quotas

| Group | Internal IDs | Format | ALL quota first | OBW | Additional BWL/ALL | SBW/ASP | Extra final batting place |
|---|---|---|---:|---:|---:|---:|---:|
| Normal/Dry/Astro/Hard | 0,2,3,5,10,11,12,13,14 | Limited | 1 | 2 | 1 | 1 | 0 |
| Dusty | 1,6,7,15 | Limited | 2 | 2 | 0 | 1 | 0 |
| Damp/Green/Wet | 4,8,9,16 | Limited | 2 | 2 | 0 | 1 | 0 |
| Normal/Dry/Astro/Hard | 0,2,3,5,10,11,12,13,14 | Test | 0 | 2 | 1 | 1 | 1 |
| Dusty | 1,6,7,15 | Test | 0 | 2 | 0 | 2 | 1 |
| Damp/Green/Wet | 4,8,9,16 | Test | 0 | 2 | 2 | 1 | 0 |

Missing OBW seats are added to the Additional BWL/ALL count. The total bowling/ALL selection count stays five in limited overs, four on Normal/Dusty Tests, and five on Damp/Green/Wet Tests. The remaining places are filled by keeper/openers/batting.

Selection order and bowling-order placement are separate. Even though ALL quota is picked first, the bowling order starts with the selected OBW and their replacements, then additional bowlers, spinners, and ALL-quota players. Original native completion fills the sixth bowling-order place from the XI, excluding the designated keeper. Captain selection, reserve metadata and native finalization are retained.

## What changes from v2.2

- Keeper/openers/batters are no longer protected from earlier Accuracy picks.
- Top ALL/ASP are reserved first, with no batting-credit substitutions.
- A fallback of any nominal role remains an ALL-quota selection.
- Missing OBW places are deferred into BWL/ALL; ALL can now fill those opening-bowler vacancies before any-role fallback.
- Spinner candidates may already have been used by the ALL quota.
- Keeper priority stays the same, but operates on the remaining players.
- The keeper competes with other opener candidates instead of automatically opening on role alone.
- Final batting is last, including the broadened Test flex place.

On the extracted roster comparison against v2.2:

| Format | XI membership changed | Same XI, order-only changed | Designated keeper changed | Bowling seed changed |
|---|---:|---:|---:|---:|
| Limited | 319 / 1,071 | 0 | 17 | 767 |
| Test | 415 / 1,071 | 0 | 0 | 320 |

These are source-core comparisons across 63 eligible squads × 17 pitches per format. Bowling-seed changes are not a measurement of the complete native bowling-order output.

Observed examples, also checked inside the game:

| Squad / conditions | In | Out |
|---|---|---|
| Australia, Normal limited | S.Clark | A.Symonds |
| South Africa, Normal/Dusty Test | L.Bosman | A.Hall |
| South Africa, Green Test | A.de Villiers | J.Kemp |
| South Africa, Normal/Green/Dusty limited | Same XI | Same XI |
| India, Normal/Green/Dusty, both formats | Same XI | Same XI |

South Africa's limited XI retains de Villiers, but for a different reason: Kallis is now selected through ALL quota first, leaving Gibbs/Rudolph/de Villiers as the three batting picks.

## Build and reproduce

From the workspace root in PowerShell:

    & .\plugins\Cricket07Lineup\build.ps1
    & .\plugins\Cricket07Lineup\build.ps1 -Install
    & .\plugins\Cricket07Lineup\compare-v2.2.ps1

The last command compiles the saved v2.2 core under a different symbol and compares both implementations on the same extracted CDF. Its output is saved in tests/roster-compare-v3.0.txt for this release.

The build uses the existing Zig 0.15.2 x86 compiler. -Zig and -RosterPath override compiler and extracted CDF. Close the game before installation. Recognized earlier versions are backed up under releases/ before replacement; an unrecognized installed ASI is refused.

Supported EXE SHA-256: d776dba0cc506b968f7d228e036cf42aeb4abe45a8d1ad0d7226cdebf91553a2.

ASI SHA-256: 1ca980c7ac867461eb128799f46772e680e15d8af0ebc52004d9c331c6acece4.

Cricket07Lineup.log reports version, installation status and automatic selections. Unsupported EXEs/conflicting entry bytes are refused. Unexpected selector/roster input logs FALLBACK and uses the original builder for that request.

Uninstall: close Cricket07 and remove only E:\Cricket07\Cricket07Lineup.asi. Keep ddraw.dll and other ASIs. The optional lineup log may also be removed.

## Source and evidence

src/selection.c contains the full c07_select core. src/plugin.c reads live roster values, writes the completed XI/bench, and calls original bowling completion 0x413400, captain 0x4DA740 and finalizer 0x414950. The only detour remains VA 0x417330 / RVA 0x17330. Original wrapper 0x4180D0 is used in controlled game tests.

The former role-selection helpers 0x4161F0/0x416370/0x416610/0x416890 are not called. The requested pools/fallbacks and retained pitch quotas are implemented in the core. Hardcoded player-ID exclusions are not applied.

Skill and Accuracy use stored six-bit ratings. Player lookup is 0x4D8F10. Record fields: role +0x15F & 15; Skill (u32(+0x6C)>>6)&63; Accuracy (u32(+0x74)>>12)&63; keeper field-position tiers in +0x84; legacy keeper field u32(+0x8C)&63.

See [VALIDATION.md](VALIDATION.md) for the 6,000 varied-squad, 2,142 roster, 136 x86 adapter and 36 in-game checks, plus their limits.
