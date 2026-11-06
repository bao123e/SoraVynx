$ErrorActionPreference = 'Stop'
$root = Join-Path $env:TEMP ('soravynx-helper-test-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $root | Out-Null
$fakeScript = Join-Path $root 'FakeAdb.ps1'
@'
$ErrorActionPreference = 'Stop'
$a = @($args)
$fixture = $env:ADB_FIXTURE
$a | ConvertTo-Json -Compress | Add-Content -LiteralPath (Join-Path $fixture 'trace.txt')
if ($a[0] -eq 'devices') { Write-Output "List of devices attached`ntest-device`tdevice"; exit 0 }
if ($a[0] -ne '-s' -or $a[1] -ne 'test-device') { exit 2 }
$a = $a[2..($a.Count-1)]
if ($a[0] -eq 'shell') {
    if ($a[1] -eq 'id') { Write-Output '2000'; exit 0 }
    if ($a[1] -eq 'pm') { Write-Output 'package:/data/app/game/base.apk'; exit 0 }
    if ($a[1] -eq 'pidof') {
        if ($env:ADB_SCENARIO -eq 'running') { Write-Output '1234'; exit 0 }
        exit 1
    }
    $command = $a[1]
    if ($command -notmatch "'/sdcard/Android/(data|obb)/com.YoStarEN.StellaSora'") { throw 'Unexpected shell command' }
    $kind = $Matches[1]
    $remote = '/sdcard/Android/' + $kind + '/com.YoStarEN.StellaSora'
    $path = Join-Path $fixture ('phone\' + $kind + '\com.YoStarEN.StellaSora')
    if ($command.StartsWith('if ')) {
        if (Test-Path -LiteralPath $path) {
            $bytes = (Get-ChildItem -LiteralPath $path -Recurse -File | Measure-Object -Property Length -Sum).Sum
            Write-Output (([Math]::Max(1,[Math]::Ceiling($bytes/1024))).ToString() + "`t" + $remote)
        } else { Write-Output 'MISSING' }
        exit 0
    }
    if ($command.StartsWith('find ')) {
        foreach ($file in Get-ChildItem -LiteralPath $path -Recurse -File) {
            $marker = '\com.YoStarEN.StellaSora\'
            $start = $file.FullName.LastIndexOf($marker, [StringComparison]::OrdinalIgnoreCase)
            if ($start -lt 0) { throw 'Unexpected fixture path' }
            $suffix = $file.FullName.Substring($start + $marker.Length).Replace('\','/')
            Write-Output ((Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant() + '  ' + $remote + '/' + $suffix)
        }
        exit 0
    }
    throw 'Unexpected shell operation'
}
if ($a[0] -eq 'pull' -and $a[1] -eq '-a') {
    if ($a[2] -notmatch '^/sdcard/Android/(data|obb)/com.YoStarEN.StellaSora$') { exit 2 }
    $path = Join-Path $fixture ('phone\' + $Matches[1] + '\com.YoStarEN.StellaSora')
    Copy-Item -LiteralPath $path -Destination $a[3] -Recurse -Force
    if ($env:ADB_SCENARIO -eq 'corrupt') {
        Set-Content -LiteralPath (Join-Path $a[3] 'com.YoStarEN.StellaSora\files\resource.bundle') -Value 'corrupt'
    }
    if ($env:ADB_SCENARIO -eq 'changed') {
        Set-Content -LiteralPath (Join-Path $path 'files\resource.bundle') -Value 'changed on phone'
    }
    exit 0
}
throw 'Unexpected ADB mutation attempted'
'@ | Set-Content -LiteralPath $fakeScript -Encoding UTF8
$fakeCmd = Join-Path $root 'adb.cmd'
('@echo off' + "`r`n" + 'powershell.exe -NoProfile -ExecutionPolicy Bypass -File "' + $fakeScript + '" %*') | Set-Content -LiteralPath $fakeCmd -Encoding ASCII
$helper = Join-Path $PSScriptRoot '..\data-helper\GameData.ps1'
foreach ($case in @('inspect','good','corrupt','changed','running')) {
    $fixture = Join-Path $root $case
    $data = Join-Path $fixture 'phone\data\com.YoStarEN.StellaSora\files'
    New-Item -ItemType Directory -Path $data -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $data 'resource.bundle') -Value 'known resource bytes'
    $env:ADB_FIXTURE = $fixture; $env:ADB_SCENARIO = $case
    $mode = if ($case -eq 'inspect') { 'Inspect' } else { 'Backup' }
    $destination = Join-Path $fixture 'backup with spaces'
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $helper -Mode $mode -AdbPath $fakeCmd -BackupDirectory $destination
    $code = $LASTEXITCODE
    $manifests = @(Get-ChildItem -LiteralPath $fixture -Filter manifest.json -File -Recurse)
    $trace = Get-Content -LiteralPath (Join-Path $fixture 'trace.txt') -Raw
    if ($trace -match 'uninstall|force-stop|"push"|"install"') { throw 'Disallowed phone mutation' }
    if ($case -eq 'inspect') {
        if ($code -ne 0 -or $trace -match '"pull"' -or $manifests.Count) { throw 'Inspect must remain read only' }
    } elseif ($case -eq 'good') {
        if ($code -ne 0 -or $manifests.Count -ne 1) { throw 'Valid backup did not complete' }
        $manifest = Get-Content -LiteralPath $manifests[0].FullName -Raw | ConvertFrom-Json
        if ($manifest.FileCount -ne 1 -or $manifest.TotalBytes -le 0) { throw 'Invalid verified manifest' }
        if (@(Get-ChildItem -LiteralPath $destination -Filter INCOMPLETE.txt -Recurse).Count) { throw 'Complete backup still marked incomplete' }
    } else {
        if ($code -eq 0 -or $manifests.Count) { throw "Unsafe backup was accepted: $case" }
        if ($case -eq 'running' -and $trace -match '"pull"') { throw 'Running game must not be copied' }
    }
    Write-Output ("PASS data helper: " + $case)
}

$global:LASTEXITCODE = 0
