param([string]$Zig, [string]$RosterPath, [switch]$Install)
$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
if (-not $Zig) { $Zig = Join-Path $projectRoot '..\..\work\toolchains\zig-x86_64-windows-0.15.2\zig.exe' }
$Zig = (Resolve-Path -LiteralPath $Zig).Path
$dist = Join-Path $projectRoot 'dist'
$testDir = Join-Path $projectRoot 'tests\bin'
New-Item -ItemType Directory -Force -Path $dist,$testDir | Out-Null
$env:ZIG_GLOBAL_CACHE_DIR = Join-Path $projectRoot '..\..\work\toolchains\zig-cache'
$src = Join-Path $projectRoot 'src'
$plugin = Join-Path $dist 'Cricket07Lineup.asi'
& $Zig cc -target x86-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -shared `
    (Join-Path $src 'plugin.c') (Join-Path $src 'selection.c') (Join-Path $src 'hook.c') `
    (Join-Path $src 'exports.def') -ladvapi32 -lkernel32 -o $plugin
if ($LASTEXITCODE) { throw 'ASI compilation failed.' }
$coreTest = Join-Path $testDir 'core_test.exe'
& $Zig cc -target x86-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror `
    (Join-Path $projectRoot 'tests\core_test.c') (Join-Path $src 'selection.c') -o $coreTest
if ($LASTEXITCODE) { throw 'Core test compilation failed.' }
& $coreTest
if ($LASTEXITCODE) { throw 'Core selection tests failed.' }
$liveSquadTest = Join-Path $testDir 'live_squad_test.exe'
& $Zig cc -target x86-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror `
    (Join-Path $projectRoot 'tests\live_squad_test.c') (Join-Path $src 'selection.c') -o $liveSquadTest
if ($LASTEXITCODE) { throw 'Live squad fixture compilation failed.' }
& $liveSquadTest
if ($LASTEXITCODE) { throw 'Live squad fixture failed.' }
$rosterTest = Join-Path $testDir 'roster_test.exe'
if (-not $RosterPath) {
    $RosterPath = Join-Path $projectRoot '..\..\analysis\batting_20261021\frontend__database__cricket.0.0.12.0.cdf'
}
$RosterPath = (Resolve-Path -LiteralPath $RosterPath).Path
& $Zig cc -target x86-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror `
    (Join-Path $projectRoot 'tests\roster_test.c') (Join-Path $src 'selection.c') -o $rosterTest
if ($LASTEXITCODE) { throw 'Roster test compilation failed.' }
& $rosterTest $RosterPath
if ($LASTEXITCODE) { throw 'Roster reference replay failed.' }
$nativeTest = Join-Path $testDir 'native_test.exe'
& $Zig cc -target x86-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror `
    (Join-Path $projectRoot 'tests\native_test.c') (Join-Path $src 'selection.c') `
    (Join-Path $src 'hook.c') -ladvapi32 -lkernel32 -o $nativeTest
if ($LASTEXITCODE) { throw 'Native test compilation failed.' }
& $nativeTest $plugin
if ($LASTEXITCODE) { throw 'Native integration tests failed.' }
Get-FileHash -LiteralPath $plugin -Algorithm SHA256
if ($Install) {
    $game = 'E:\Cricket07'
    if (Get-Process Cricket07 -ErrorAction SilentlyContinue) { throw 'Close Cricket07 before installation.' }
    $expected = 'D776DBA0CC506B968F7D228E036CF42AEB4ABE45A8D1AD0D7226CDEBF91553A2'
    if ((Get-FileHash -LiteralPath (Join-Path $game 'Cricket07.exe') -Algorithm SHA256).Hash -ne $expected) {
        throw 'Unsupported executable; installation stopped.'
    }
    $destination = Join-Path $game 'Cricket07Lineup.asi'
    if (Test-Path -LiteralPath $destination) {
        $installedHash = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash
        $newHash = (Get-FileHash -LiteralPath $plugin -Algorithm SHA256).Hash
        if ($installedHash -ne $newHash) {
            $knownVersions = @{
                '4F1568CC3B0F195103B3DCABF1EFE18B2BA3BA5179F814C795D9096AB0360362' = '1.0'
                '4102498252937A2A5A0C1F8B4AA0B969838F989377F94CD06B4370D899AE79A5' = '1.1'
                '8FFD6B19E9067E0F3C6D2A59AC10D3E9637D50B135D2E338D419EB64427C11AC' = '1.2'
                'D09AB092985187C6534486EC970CD6AFC44D9BF643DAE4F5A46B39DAECCFB1CB' = '1.3'
                'BACBA6C6ED903FE4CD19815A2F7674D3A311EA1BA9C97C6DC90051B8F01AE26F' = '2.0'
                'D20BD67D89462023A658B1135D199E586D998A37480DCC01DE87EE3E5887D568' = '2.1'
            }
            $oldVersion = $knownVersions[$installedHash]
            if (-not $oldVersion) {
                throw 'An unrecognized Cricket07Lineup.asi is installed; preserve it before replacing.'
            }
            $backupDir = Join-Path $projectRoot ('releases\' + $oldVersion)
            $backup = Join-Path $backupDir 'Cricket07Lineup.asi'
            New-Item -ItemType Directory -Path $backupDir -Force | Out-Null
            if (-not (Test-Path -LiteralPath $backup)) {
                Copy-Item -LiteralPath $destination -Destination $backup
            }
            if ((Get-FileHash -LiteralPath $backup -Algorithm SHA256).Hash -ne $installedHash) {
                throw 'Previous-version backup differs from the installed file; upgrade stopped.'
            }
            Copy-Item -LiteralPath $plugin -Destination $destination -Force
            Write-Output ('Upgraded version ' + $oldVersion + '; previous ASI preserved in ' + $backupDir)
        } else {
            Write-Output 'Identical build already installed.'
        }
    } else {
        Copy-Item -LiteralPath $plugin -Destination $destination
    }
    Get-FileHash -LiteralPath $destination -Algorithm SHA256
}
