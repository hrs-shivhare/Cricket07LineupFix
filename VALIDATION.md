# Validation - 25 October 2026

Build: Cricket07Lineup 3.2. SHA-256: 3b52a667e4877ab8e5cc7c36c9760b9335d61d1a29b44608434cec1669f46b38.

This revision changes executable compatibility and logging only. src/selection.c is byte-identical to v3.1 (SHA-256 1dde0254a01f27f0c3507a9f284599bc8f605ac70d2647b625031977c6a2d1e5). The lineup algorithm is unchanged.

| Check | Coverage | Result |
|---|---|---|
| Selection core | Directed rules and 6,000 varied squads | Pass |
| Captured live roster | South Africa frozen 20-player fixture | Pass |
| Extracted roster | 63 squads x 17 pitches x 2 formats = 2,142 cases | Pass |
| Native adapter | 136 cases, original completion/captain/finalization stubs, x86 detour/trampoline, wrong-host refusal | Pass |
| Compatibility fixtures | 48 checks: compatible code/larger image accepted; unrelated changes accepted; first/last-byte changes in all 17 required body ranges refused; PE32/base/size/readability/executable protection failures refused | Pass |
| Actual EXE file bytes | Reference EXE privately mapped and checked | Pass |
| Changed file identity | Private EXE copy with appended overlay and different SHA-256 | Compatible |
| Changed required code | Private copy with changed byte at RVA 0x17338 | Refused with function VA/RVA |
| Actual modified-EXE startup | Copy with overlay; ordinary ASI Loader loads v3.2 | Hash WARNING, COMPATIBLE, APPLIED |
| Actual incompatible-EXE startup | Copy with changed lineup-function byte | Hash WARNING, NOT APPLIED at RVA 0x17338 |
| Installation | Tested dist ASI and installed ASI hashes match | Pass |
| Protected files | EXE, data.gob, ddraw.dll and five other installed ASIs | All 8 unchanged |

The two startup tests used isolated folders containing private EXE copies, the existing loader DLL and the new ASI, with E:\Cricket07 as working directory. They made no match selections and closed only their own two processes. The installed executable and roster were not edited. Startup acceptance proves the new warning/check/hook initialization path on these two concrete variants, not every third-party modification or long-running match stability.

## Code checks

Signatures are exact recovered body ranges from the reference EXE, not semantic function equivalence:

- 0x40BAD0 mode getter; 0x413400 bowling completion; 0x414950 finalizer; 0x417330 original lineup builder.
- 0x487C10 pitch and 0x487C30 format getters.
- 0x4D8F10 player lookup; 0x4DA740 captain; 0x4E04B0 team lookup; 0x4E0550 squad count.
- Supporting getters 0x4758C0 / 0x4758E0 / 0x6E5BD0, RTTI helper 0x749001 and security-cookie helper 0x74919B.

There are 15 functions in 17 discontiguous ranges. The original reference has no PE base relocations; this version requires its original fixed load base and addresses. Added sections / larger image sizes are allowed if required code and data remain available. The checks do not cover every transitive callee or prove unchanged runtime object semantics in arbitrary mods. Missing code or a hook inside these ranges is rejected rather than overwritten.

## Reproduce and evidence

- build.ps1: builds/runs all automated checks, warns on an EXE hash mismatch during installation, preserves known old ASIs.
- generate-compatibility.ps1: regenerates src/compatibility_data.h from the exact reference hash and existing Ghidra metadata.
- tests/compatibility_test.c: synthetic compatibility cases and optional --check-file private mapping.
- tests/startup_compatibility.ps1: optional two-copy startup test; refuses to run while an existing game is running and preserves existing logs.
- tests/build-v3.2.txt: build/test/install output.
- tests/compatibility-files-v3.2.txt and compatibility-files-v3.2-hashes.json: original/changed-file compatibility and hashes.
- tests/startup-compatibility-v3.2.json: actual startup logs and process IDs for both variants.
- tests/host-refusal-v3.2.log: unrelated host hash warns, then layout incompatibility disables plugin.
- tests/protected-files-v3.2-before.json and protected-files-v3.2-after.json: matching fingerprints.
- releases/3.1/: previous ASI, sources, tests and documentation.

The earlier v3.1/v3.0 lineup comparisons remain historical evidence, unchanged by this release. There is no Frida/REA dependency in the installed ASI.