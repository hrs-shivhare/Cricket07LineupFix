# Optional controlled startup check. Uses private EXE copies and closes only
# the processes it creates. Does not edit the installed EXE or launch a match.
$ErrorActionPreference = 'Stop'
if (Get-Process Cricket07 -ErrorAction SilentlyContinue) { throw 'An existing game is running; do not disturb it.' }
$project = Split-Path -Parent $PSScriptRoot
$fixtures = Join-Path $PSScriptRoot 'compatibility-fixtures'
$results = @()
foreach ($case in @(
    @{Name='allowed'; Source='Cricket07-overlay-modified.exe'; Applied=$true},
    @{Name='refused'; Source='Cricket07-lineup-modified.exe'; Applied=$false}
)) {
    $folder = Join-Path $fixtures ('startup-' + $case.Name)
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    $log = Join-Path $folder 'Cricket07Lineup.log'
    if (Test-Path -LiteralPath $log) { throw ('Existing startup log must be preserved first: ' + $log) }
    Copy-Item -LiteralPath (Join-Path $fixtures $case.Source) -Destination (Join-Path $folder 'Cricket07.exe') -Force
    Copy-Item -LiteralPath 'E:\Cricket07\ddraw.dll' -Destination $folder -Force
    Copy-Item -LiteralPath (Join-Path $project 'dist\Cricket07Lineup.asi') -Destination $folder -Force
    $process = Start-Process -FilePath (Join-Path $folder 'Cricket07.exe') -WorkingDirectory 'E:\Cricket07' -WindowStyle Hidden -PassThru
    try {
        $content = ''
        for ($i=0; $i -lt 40; ++$i) {
            if (Test-Path -LiteralPath $log) { $content = Get-Content -LiteralPath $log -Raw }
            if ($content -match '(?m)^APPLIED:|^NOT APPLIED:') { break }
            if ($process.HasExited) { break }
            Start-Sleep -Milliseconds 500
        }
        $applied = $content -match '(?m)^APPLIED:'
        if ($content -notmatch 'WARNING: executable SHA-256 differs') { throw ('Missing mismatch warning: ' + $case.Name) }
        if ($applied -ne $case.Applied) { throw ('Unexpected hook status: ' + $case.Name + "`n" + $content) }
        if (!$applied -and $content -notmatch 'required function VA 0x00417330 differs at RVA 0x00017338') {
            throw ('Missing specific function-refusal reason: ' + $content)
        }
        $results += [PSCustomObject]@{Case=$case.Name; PID=$process.Id; Applied=$applied; Passed=$true; Log=$content}
        Write-Output ($case.Name + ': mismatch warned; applied=' + $applied)
    } finally {
        if (!$process.HasExited) { Stop-Process -Id $process.Id; $process.WaitForExit(10000) | Out-Null }
        $process.Dispose()
    }
}
$results | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'startup-compatibility-v3.2.json')
