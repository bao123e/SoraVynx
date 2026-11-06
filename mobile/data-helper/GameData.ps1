# SoraVynx data helper. Reads the phone; writes backups only to the computer.
# SPDX-License-Identifier: AGPL-3.0-or-later
[CmdletBinding()]
param([ValidateSet('Inspect','Backup')][string]$Mode = 'Inspect',
      [string]$BackupDirectory = '', [string]$AdbPath = '')
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$GamePackage = 'com.YoStarEN.StellaSora'
$Roots = @(
    @{ Name = 'data'; Remote = "/sdcard/Android/data/$GamePackage" },
    @{ Name = 'obb'; Remote = "/sdcard/Android/obb/$GamePackage" }
)

function Initialize-Adb {
    if ($AdbPath -and (Test-Path -LiteralPath $AdbPath)) { return $AdbPath }
    $bundled = Join-Path $PSScriptRoot 'platform-tools\adb.exe'
    if (Test-Path -LiteralPath $bundled) { return $bundled }
    $installed = Get-Command adb.exe -ErrorAction SilentlyContinue
    if ($installed) { return $installed.Source }
    Write-Host 'Dang tai Android Platform Tools tu Google (chi lan dau)...'
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    $archive = Join-Path $PSScriptRoot 'platform-tools.zip'
    Invoke-WebRequest -UseBasicParsing -Uri 'https://dl.google.com/android/repository/platform-tools-latest-windows.zip' -OutFile $archive
    Expand-Archive -LiteralPath $archive -DestinationPath $PSScriptRoot -Force
    if (!(Test-Path -LiteralPath $bundled)) { throw 'Khong tim thay adb.exe sau khi tai.' }
    return $bundled
}

function Invoke-Adb([string[]]$AdbArguments, [switch]$AllowFailure) {
    $savedPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try { $lines = @(& $script:Adb @AdbArguments 2>&1 | ForEach-Object { $_.ToString() }); $code = $LASTEXITCODE }
    finally { $ErrorActionPreference = $savedPreference }
    $text = $lines -join "`n"
    if ($code -ne 0 -and !$AllowFailure) { throw "ADB that bai ($code): $text" }
    return @{ Code = $code; Text = $text }
}

function Phone([string[]]$Command, [switch]$AllowFailure) {
    return Invoke-Adb -AdbArguments (@('-s', $script:Serial) + $Command) -AllowFailure:$AllowFailure
}

function Get-RemoteCatalog($ActiveRoots) {
    $catalog = @{}
    $caseNames = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($root in $ActiveRoots) {
        $remote = $root.Remote
        $listing = Phone -Command @('shell', "find '$remote' -type f -exec sha256sum {} \;")
        foreach ($line in ($listing.Text -split "`r?`n")) {
            if (!$line.Trim()) { continue }
            if ($line -notmatch '^([0-9a-fA-F]{64})\s+(.+)$') { throw "Khong doc duoc hash tep: $line" }
            $hash = $Matches[1].ToLowerInvariant(); $path = $Matches[2]
            if (!$path.StartsWith($remote + '/', [StringComparison]::Ordinal)) { throw 'Duong dan ngoai thu muc game.' }
            $suffix = $path.Substring($remote.Length + 1)
            foreach ($part in ($suffix -split '/')) {
                if (!$part -or $part -eq '.' -or $part -eq '..' -or $part.IndexOfAny([IO.Path]::GetInvalidFileNameChars()) -ge 0) {
                    throw 'Ten tep khong sao luu an toan duoc tren Windows.'
                }
            }
            $relative = $root.Name + '/' + $GamePackage + '/' + $suffix
            if (!$caseNames.Add($relative)) { throw 'Ten tep trung nhau tren Windows; dung de tranh mat du lieu.' }
            $catalog.Add($relative, $hash)
        }
    }
    return ,$catalog
}

try {
    $script:Adb = Initialize-Adb
    $deviceList = (Invoke-Adb -AdbArguments @('devices')).Text
    $devices = @([regex]::Matches($deviceList, '(?m)^([^\s]+)\s+device\s*$') | ForEach-Object { $_.Groups[1].Value })
    if ($devices.Count -ne 1) {
        throw "Can dung 1 dien thoai da cho phep USB debugging. Kiem tra cap USB va bam Allow tren dien thoai.`n$deviceList"
    }
    $script:Serial = $devices[0]
    $userId = (Phone -Command @('shell','id','-u')).Text.Trim()
    if ($userId -ne '2000') { throw 'Cong cu nay chi kiem tra ADB shell khong root (uid 2000).' }
    $packagePath = Phone -Command @('shell','pm','path',$GamePackage) -AllowFailure
    if ($packagePath.Text -notmatch 'package:') { throw 'Chua tim thay game Stella Sora dang cai.' }
    $active = @(); [long]$estimatedBytes = 0
    foreach ($root in $Roots) {
        $probe = Phone -Command @('shell', "if [ -d '$($root.Remote)' ]; then du -sk '$($root.Remote)'; else echo MISSING; fi") -AllowFailure
        if ($probe.Code -ne 0 -or $probe.Text -match 'Permission denied|Operation not permitted') {
            throw "Dien thoai khong cho ADB doc $($root.Remote). Chua co ban sao luu; khong go game."
        }
        if ($probe.Text -match '(?m)^(\d+)\s+') {
            $bytes = [long]$Matches[1] * 1024
            $estimatedBytes += $bytes; $active += $root
            Write-Host ('Doc duoc {0}: {1:N2} GB' -f $root.Remote, ($bytes / 1000000000))
        } else { Write-Host ('Chua co thu muc: ' + $root.Remote) }
    }
    Write-Host ('TONG DU LIEU NGOAI DOC DUOC: {0:N2} GB' -f ($estimatedBytes / 1000000000))
    Write-Host 'Ket qua KHONG bao gom /data/user/0 (du lieu rieng, dang nhap).'
    Write-Host 'Chua the ket luan day la toan bo 12 GB. Inspect khong sua hoac xoa tep tren dien thoai.'
    if ($Mode -eq 'Inspect') { exit 0 }
    if (!$active.Count -or !$estimatedBytes) { throw 'Khong co du lieu ngoai de sao luu. Khong go game.' }
    $running = Phone -Command @('shell','pidof',$GamePackage) -AllowFailure
    if ($running.Text.Trim()) {
        throw 'Game dang chay. Cho tai xong, dong Menu va Force stop game trong Settings truoc khi chay Backup. Cong cu khong tu dung game.'
    }
    if (!$BackupDirectory) { $BackupDirectory = Join-Path $PSScriptRoot 'backups' }
    $backupBase = [IO.Path]::GetFullPath($BackupDirectory)
    $drive = [IO.DriveInfo]::new([IO.Path]::GetPathRoot($backupBase))
    if ($drive.AvailableFreeSpace -lt ($estimatedBytes * 1.1 + 536870912)) { throw 'O dia PC khong du cho trong de sao luu.' }
    $snapshot = Join-Path $backupBase ([DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss') + '-' + [Guid]::NewGuid().ToString('N').Substring(0,8))
    New-Item -ItemType Directory -Path $snapshot -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $snapshot 'INCOMPLETE.txt') -Value 'Backup has not passed integrity checks.' -Encoding UTF8
    Write-Host 'Dang tinh hash nguon. Voi 12 GB, buoc nay co the mat vai phut...'
    $before = Get-RemoteCatalog $active
    if (!$before.Count) { throw 'Thu muc khong chua tep; chua sao luu duoc du lieu.' }
    foreach ($root in $active) {
        $destination = Join-Path $snapshot $root.Name
        New-Item -ItemType Directory -Path $destination -Force | Out-Null
        Write-Host ('Dang sao chep ' + $root.Remote)
        Phone -Command @('pull','-a',$root.Remote,$destination) | Out-Null
    }
    $files = @(); [long]$actualBytes = 0
    foreach ($relative in $before.Keys) {
        $local = Join-Path $snapshot ($relative.Replace('/',[IO.Path]::DirectorySeparatorChar))
        if (!(Test-Path -LiteralPath $local -PathType Leaf)) { throw "Thieu tep sao luu: $relative" }
        $hash = (Get-FileHash -LiteralPath $local -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($hash -ne $before[$relative]) { throw "Hash khong khop: $relative. Khong go game." }
        $length = (Get-Item -LiteralPath $local).Length; $actualBytes += $length
        $files += @{ Path=$relative; Sha256=$hash; Bytes=$length }
    }
    $after = Get-RemoteCatalog $active
    if ($after.Count -ne $before.Count) { throw 'Du lieu game thay doi trong luc sao luu. Khong go game.' }
    foreach ($relative in $before.Keys) {
        if (!$after.ContainsKey($relative) -or $after[$relative] -ne $before[$relative]) { throw 'Du lieu nguon da thay doi. Khong go game.' }
    }
    $manifest = @{ SchemaVersion=1; Package=$GamePackage; CreatedUtc=[DateTime]::UtcNow.ToString('o');
        TotalBytes=$actualBytes; FileCount=$files.Count; Files=$files; Coverage='External data and OBB only. Private app data excluded.' }
    $manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $snapshot 'manifest.json') -Encoding UTF8
    Remove-Item -LiteralPath (Join-Path $snapshot 'INCOMPLETE.txt')
    Write-Host ('SAO LUU DA DOI CHIEU HASH: {0} tep, {1:N2} GB' -f $files.Count, ($actualBytes / 1000000000))
    Write-Host ('Thu muc PC: ' + $snapshot)
    Write-Host 'Ban sao luu nay chi chung minh cac tep ben ngoai da duoc copy. Chua bao dam game se nhan lai cache; khong tu go/cai APK.'
} catch {
    Write-Host ('DUNG: ' + $_.Exception.Message) -ForegroundColor Red
    Write-Host 'Khong go game va khong xoa du lieu. Khong co lenh uninstall/install/push trong cong cu nay.'
    exit 1
}
