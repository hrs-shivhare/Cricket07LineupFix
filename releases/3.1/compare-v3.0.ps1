param([string]$Zig,[string]$RosterPath)
$ErrorActionPreference = 'Stop'
if (-not $Zig) { $Zig = Join-Path $PSScriptRoot '..\..\work\toolchains\zig-x86_64-windows-0.15.2\zig.exe' }
if (-not $RosterPath) { $RosterPath = Join-Path $PSScriptRoot '..\..\analysis\batting_20261021\frontend__database__cricket.0.0.12.0.cdf' }
$env:ZIG_GLOBAL_CACHE_DIR = Join-Path $PSScriptRoot '..\..\work\toolchains\zig-cache'
$compareBin = Join-Path $PSScriptRoot 'tests\bin'
New-Item -ItemType Directory -Path $compareBin -Force | Out-Null
$oldObject = Join-Path $compareBin 'selection-v3.0.o'
$executable = Join-Path $compareBin 'compare-v3.0.exe'
& $Zig cc -target x86-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -Dc07_select=c07_select_v30 `
    -c (Join-Path $PSScriptRoot 'releases\3.0\src\selection.c') -o $oldObject
if ($LASTEXITCODE) { throw 'Historical core compilation failed.' }
& $Zig cc -target x86-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -DC07_COMPARE_V30 `
    (Join-Path $PSScriptRoot 'tests\roster_test.c') (Join-Path $PSScriptRoot 'src\selection.c') `
    $oldObject -o $executable
if ($LASTEXITCODE) { throw 'Comparison compilation failed.' }
& $executable $RosterPath
if ($LASTEXITCODE) { throw 'Roster comparison failed.' }
