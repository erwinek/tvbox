#Requires -Version 5.1
<#
.SYNOPSIS
  Nadpisuje USB czystym Ubuntu live-server ISO (+ druga partycja TVBOXIMG ze skryptami).

.PARAMETER IsoPath
  Sciezka do ISO (domyslnie Downloads\ubuntu-26.04-live-server-amd64.iso)

.PARAMETER Confirm
  Wymagane — bez tego nic nie zapisze.
#>
param(
    [string]$IsoPath = "",
    [switch]$Confirm
)

$ErrorActionPreference = "Stop"
$ScriptDir = $PSScriptRoot

if (-not $IsoPath) {
    $IsoPath = Join-Path $env:USERPROFILE "Downloads\ubuntu-26.04-live-server-amd64.iso"
}
if (-not (Test-Path -LiteralPath $IsoPath)) {
    throw "Brak ISO: $IsoPath"
}
$isoSize = (Get-Item -LiteralPath $IsoPath).Length
Write-Host "ISO: $IsoPath ($([math]::Round($isoSize/1GB,2)) GB)"

$usb = Get-Disk | Where-Object { $_.BusType -eq 'USB' } | Select-Object -First 1
if (-not $usb) { throw "Brak dysku USB — wloz pendrive do PC." }
if ($usb.Size -lt 8GB) { throw "USB za maly." }

Write-Host "USB Disk#$($usb.Number) $($usb.FriendlyName) $([math]::Round($usb.Size/1GB,1)) GB"
if (-not $Confirm) {
    throw "Podaj -Confirm (nadpisze caly USB)."
}
$ans = Read-Host "Wpisz YES aby wymazac USB i wgrac Ubuntu ISO"
if ($ans -ne 'YES') { throw 'Anulowano' }

# Odmontuj wolumeny
Get-Partition -DiskNumber $usb.Number -ErrorAction SilentlyContinue | ForEach-Object {
    if ($_.DriveLetter) {
        try { Remove-PartitionAccessPath -DiskNumber $usb.Number -PartitionNumber $_.PartitionNumber -AccessPath "$($_.DriveLetter):\" -ErrorAction SilentlyContinue } catch {}
    }
}

$dd = "C:\Program Files\Git\usr\bin\dd.exe"
if (-not (Test-Path $dd)) { $dd = "C:\ProgramData\chocolatey\bin\dd.exe" }
if (-not (Test-Path $dd)) { throw "Brak dd.exe (Git lub Chocolatey)" }

$of = "\\.\PhysicalDrive$($usb.Number)"
Write-Host "Writing ISO via dd -> $of (wymaga Admin)..."
$ddArgs = "if=$IsoPath of=$of bs=4M status=progress oflag=disk"
# Elevate
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $dd
$psi.Arguments = $ddArgs
$psi.Verb = "runas"
$psi.UseShellExecute = $true
$p = [Diagnostics.Process]::Start($psi)
$p.WaitForExit()
if ($p.ExitCode -ne 0) { throw "dd exit $($p.ExitCode)" }

Write-Host "Sync/partprobe..."
Start-Sleep -Seconds 3
Update-Disk -Number $usb.Number -ErrorAction SilentlyContinue
Get-Disk -Number $usb.Number | Format-Table Number,Size,PartitionStyle
Get-Partition -DiskNumber $usb.Number | Format-Table PartitionNumber,Size,Type,DriveLetter

# Druga partycja w wolnym miejscu (jesli ISO zostawilo)
$disk = Get-Disk -Number $usb.Number
$parts = @(Get-Partition -DiskNumber $usb.Number)
$used = ($parts | Measure-Object -Property Size -Sum).Sum
$free = $disk.Size - $used
Write-Host "Free after ISO: $([math]::Round($free/1GB,2)) GB"

if ($free -gt 2GB) {
    Write-Host "Tworze partycje TVBOXIMG (exFAT) na reszcie dysku..."
    $script = @"
select disk $($usb.Number)
create partition primary
format fs=exfat label=TVBOXIMG quick
assign
exit
"@
    $dp = Join-Path $env:TEMP "tvbox-diskpart.txt"
    Set-Content -Path $dp -Value $script -Encoding ASCII
    $p2 = Start-Process -FilePath diskpart.exe -ArgumentList "/s `"$dp`"" -Verb RunAs -Wait -PassThru
    Write-Host "diskpart exit $($p2.ExitCode)"
    Start-Sleep -Seconds 3
    $tv = Get-Volume | Where-Object { $_.FileSystemLabel -eq 'TVBOXIMG' } | Select-Object -First 1
    if ($tv -and $tv.DriveLetter) {
        $root = "$($tv.DriveLetter):"
        $dest = Join-Path $root "tvbox-installer"
        $img = Join-Path $root "images"
        New-Item -ItemType Directory -Force -Path (Join-Path $dest "scripts"), $img | Out-Null
        Copy-Item (Join-Path $ScriptDir "*.sh") (Join-Path $dest "scripts") -Force
        Copy-Item (Join-Path $ScriptDir "README.md") $dest -Force -ErrorAction SilentlyContinue
        if (Test-Path (Join-Path $ScriptDir "ventoy")) {
            Copy-Item (Join-Path $ScriptDir "ventoy") (Join-Path $dest "ventoy") -Recurse -Force -ErrorAction SilentlyContinue
        }
        Get-ChildItem $dest -Recurse -Filter *.sh | ForEach-Object {
            $t = [IO.File]::ReadAllText($_.FullName) -replace "`r`n","`n"
            [IO.File]::WriteAllText($_.FullName, $t)
        }
        Write-Host "Skrypty skopiowane na ${root}\tvbox-installer"
    } else {
        Write-Host "WARN: nie znaleziono TVBOXIMG — skrypty wgraj recznie pozniej"
    }
} else {
    Write-Host "WARN: za malo wolnego miejsca na druga partycje"
}

Write-Host "=== OK: czyste Ubuntu na USB ==="
Write-Host "Boot: Ubuntu Server live -> Help/shell albo 'Enter shell'"
Write-Host "Potem: mount TVBOXIMG i prepare/restore skrypty"
