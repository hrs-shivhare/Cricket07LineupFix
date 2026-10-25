# Cricket07Lineup 3.1

Installed at E:\Cricket07\Cricket07Lineup.asi. Ultimate ASI Loader loads it on the next launch. This release moves the applicable Test batting flex place ahead of bowling selection and restricts that early pick to ALL/BAT. Executable and roster files remain unchanged. Version 3.0 is preserved in releases/3.0/.

## Selection order

Every new pick excludes all players already selected. A player's selection quota is determined by the step that picked him, not by his nominal roster role.

1. Limited overs only: select the pitch's one/two ALL/ASP quota players by bowling Accuracy. When the role pool is empty, fill its remaining seats from any role by Accuracy. Any fallback role still occupies this quota and is unavailable to later steps.
1.1. Tests only, applicable pitches: reserve one player from ALL/BAT by batting Skill before bowling selection. This player is unavailable to later pools and is not automatically added to the bowling-order seed. If no ALL/BAT exists, carry the unfilled place to the final broad batting pool.
2. Select up to two remaining OBW by Accuracy. Leave missing seats for step 3.
3. Add missing OBW seats to the additional-bowler quota. Fill this combined count from remaining BWL/ALL by Accuracy, then any role by Accuracy. First replacements complete the opening-bowling pair.
4. Select the pitch/format's one/two remaining SBW/ASP by Accuracy, then any role by Accuracy.
5. Choose the best batting keeper from the remaining squad using the first nonempty tier: WK/WKB/OWK; primary wicketkeeping field position; secondary wicketkeeping field position; legacy keeper field equal to 4; any role. Highest Skill wins within a tier; equal Skill retains the last squad entry.
6. Choose the two best remaining OBT/OWK by Skill, also considering the designated keeper if he has an opening role. The keeper opens only if he wins one of these two places. Missing opener places carry to step 7.
7. Select 3 batters, or 4 if the keeper won an opening place, plus missing opener places, from OBT/WK/WKB/OWK/BAT/ALL/ASP by Skill. A Test flex already selected in step 1.1 is not added again. Only if step 1.1 could not find ALL/BAT is that unfilled place added here. Exhaust the broad pool, then fill remaining XI shortages from any role by Skill.

Final batting order: **all selected OBT/OWK first by descending Skill, then everyone else by descending Skill**. The two chosen in step 6 are not position-locked. Equal ratings retain squad order, except the keeper tie noted above.

## Quotas

| Group | Internal IDs | Format | ALL quota first | Early BAT/ALL flex | OBW | Additional BWL/ALL | SBW/ASP |
|---|---|---|---:|---:|---:|---:|---:|
| Normal/Dry/Astro/Hard | 0,2,3,5,10,11,12,13,14 | Limited | 1 | 0 | 2 | 1 | 1 |
| Dusty | 1,6,7,15 | Limited | 2 | 0 | 2 | 0 | 1 |
| Damp/Green/Wet | 4,8,9,16 | Limited | 2 | 0 | 2 | 0 | 1 |
| Normal/Dry/Astro/Hard | 0,2,3,5,10,11,12,13,14 | Test | 0 | 1 | 2 | 1 | 1 |
| Dusty | 1,6,7,15 | Test | 0 | 1 | 2 | 0 | 2 |
| Damp/Green/Wet | 4,8,9,16 | Test | 0 | 0 | 2 | 2 | 1 |

Missing OBW seats increase the additional BWL/ALL count. Quota numbers did not change from v3.0; only the applicable Test flex pool and its selection priority changed.

Bowling-order placement remains separate from selection priority: OBW/replacements, additional bowlers, spinners, then limited-overs ALL-quota picks. Original native completion fills to six bowling-order players from the XI, excluding the designated keeper. A Test flex player may still be added by that native completion. Captain selection, reserves and native finalization are retained.

## Changes from v3.0

Previously the Test flex place was one extra late pick from the seven-role broad batting pool. It is now a separate ALL/BAT-only pick before bowling. A high-Skill ALL reserved here cannot also occupy a later bowling quota, which can admit another bowler and displace a late batter.

Comparison on the extracted roster, 63 squads x 17 pitches per format:

| Format | XI membership changed | Same XI, order-only changed | Keeper changed | Bowling seed changed |
|---|---:|---:|---:|---:|
| Limited | 0 / 1,071 | 0 | 0 | 0 |
| Test | 71 / 1,071 | 0 | 0 | 102 |

All Green/Damp/Wet Test outputs also match v3.0 exactly. Keeper/seed counts may overlap XI changes; the seed is before native bowling completion. These counts describe this extracted roster snapshot.

Examples against v3.0:

| Squad / conditions | In | Out |
|---|---|---|
| South Africa, Normal/Dusty Test | A.Hall | L.Bosman |
| South Africa, Green Test / all limited examples | Same XI | Same XI |
| Australia and India, Normal/Green/Dusty, both formats | Same XI | Same XI |

For South Africa on Normal/Dusty Tests, Kallis wins the early ALL/BAT flex by Skill. Hall then enters through bowling Accuracy: the additional BWL/ALL place on Normal, or an exhausted spinner-pool fallback on Dusty. There are now three late batting picks instead of four, so Bosman misses out. Green Tests have no flex and retain de Villiers.

## Build and reproduce

From the workspace root in PowerShell:

    & .\plugins\Cricket07Lineup\build.ps1
    & .\plugins\Cricket07Lineup\build.ps1 -Install
    & .\plugins\Cricket07Lineup\compare-v3.0.ps1

The comparison compiles the preserved v3.0 core under a separate symbol and runs identical extracted CDF inputs through both versions. It asserts exact equality for every limited-overs case and every Green/Damp/Wet Test case. Saved output: tests/roster-compare-v3.1.txt. The older compare-v2.2.ps1 remains available for comparison with that release.

The existing Zig 0.15.2 x86 compiler is used. -Zig and -RosterPath override compiler and CDF. Close the game before installation. Recognized earlier ASIs are backed up before replacement; unrecognized builds are refused.

Supported EXE SHA-256: d776dba0cc506b968f7d228e036cf42aeb4abe45a8d1ad0d7226cdebf91553a2.

ASI SHA-256: 1a0a2bebbd022f4846346301d6bcb8c3a8b0419381a3bd516ff265f47796aa5f.

The log reports version, installation status and automatic selections. Unsupported EXEs/conflicting hooks are refused. Unexpected input logs FALLBACK and uses the original builder.

Uninstall: close Cricket07 and remove only E:\Cricket07\Cricket07Lineup.asi. Keep ddraw.dll and other ASIs. The optional lineup log may also be removed.

## Source and evidence

src/selection.c contains the full c07_select core. src/plugin.c reads live roster values, writes the completed XI/bench, and calls original bowling completion 0x413400, captain 0x4DA740 and finalizer 0x414950. The only detour remains VA 0x417330 / RVA 0x17330. Original wrapper 0x4180D0 is used in controlled game tests.

The former role-selection helpers 0x4161F0/0x416370/0x416610/0x416890 are not called. The requested pools/fallbacks and retained pitch quotas are implemented in the core. Hardcoded player-ID exclusions are not applied.

Skill and Accuracy use stored six-bit ratings. Player lookup is 0x4D8F10. Record fields: role +0x15F & 15; Skill (u32(+0x6C)>>6)&63; Accuracy (u32(+0x74)>>12)&63; keeper field-position tiers in +0x84; legacy keeper field u32(+0x8C)&63.

See [VALIDATION.md](VALIDATION.md) for the 6,000 varied-squad, 2,142 roster and 136 x86 adapter checks. This revision was not replayed inside a running game.
