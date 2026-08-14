#Requires -Version 5.1
<#
.SYNOPSIS
  Przygotowuje pendrive Ventoy z Ubuntu live + skryptami tvbox-clone.

.PARAMETER Drive
  Litera dysku USB, np. E:

.PARAMETER Confirm
  Wymagane — bez tego skrypt nie nadpisze dysku.
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$Drive,
    [switch]$Confirm
)

$ErrorActionPreference = "Stop"
$ScriptDir = $PSScriptRoot
$CacheDir = Join-Path $env:TEMP "tvbox-installer-cache"
$VentoyVer = "1.0.99"
$VentoyZip = "ventoy-$VentoyVer-windows.zip"
$VentoyUrl = "https://github.com/ventoy/Ventoy/releases/download/v$VentoyVer/$VentoyZip"
# Ubuntu 24.04.2 Desktop (~6GB)
$IsoName = "ubuntu-24.04.2-desktop-amd64.iso"
$IsoUrl = "https://releases.ubuntu.com/24.04.2/$IsoName"

$Drive = $Drive.TrimEnd('\').TrimEnd(':') + ':'
if (-not $Confirm) {
    throw "Podaj -Confirm (nadpisze caly USB $Drive)."
}

$vol = Get-Volume -DriveLetter $Drive[0] -ErrorAction Stop
$part = Get-Partition -DriveLetter $Drive[0]
$disk = Get-Disk -Number $part.DiskNumber
if ($disk.BusType -ne 'USB') {
    throw "Dysk $($disk.Number) BusType=$($disk.BusType) — oczekiwano USB. Abort."
}
if ($disk.Size -lt 28GB) {
    throw "Pendrive za maly ($([math]::Round($disk.Size/1GB,1)) GB). Min ~32 GB."
}

Write-Host "=== TVBox make_usb ==="
Write-Host "Target: $Drive Disk#$($disk.Number) $($disk.FriendlyName) $([math]::Round($disk.Size/1GB,1)) GB"
Write-Host "UWAGA: cala zawartosc USB zostanie skasowana (Ventoy)."
$ans = Read-Host "Wpisz YES aby kontynuowac"
if ($ans -ne 'YES') { throw 'Anulowano' }

New-Item -ItemType Directory -Force -Path $CacheDir | Out-Null

# --- Ventoy ---
$VentoyDir = Join-Path $CacheDir "ventoy-$VentoyVer"
if (-not (Test-Path (Join-Path $VentoyDir "Ventoy2Disk.exe"))) {
    $zipPath = Join-Path $CacheDir $VentoyZip
    if (-not (Test-Path $zipPath)) {
        Write-Host "Pobieranie Ventoy $VentoyVer..."
        curl.exe -L --fail -o $zipPath $VentoyUrl
    }
    if (Test-Path $VentoyDir) { Remove-Item -Recurse -Force $VentoyDir }
    Expand-Archive -Path $zipPath -DestinationPath $CacheDir -Force
    # zip zawiera folder ventoy-x.y.z
    $extracted = Get-ChildItem $CacheDir -Directory | Where-Object { $_.Name -like "ventoy-*" } | Select-Object -First 1
    if (-not $extracted) { throw "Nie rozpakowano Ventoy" }
    $VentoyDir = $extracted.FullName
}

$VentoyExe = Join-Path $VentoyDir "Ventoy2Disk.exe"
if (-not (Test-Path $VentoyExe)) { throw "Brak Ventoy2Disk.exe w $VentoyDir" }

Write-Host "Instalacja Ventoy CLI na Disk#$($disk.Number) /Drive:$Drive ..."
# VTOYCLI: /I install, /GPT, /Y auto-confirm (nowsze Ventoy)
$cliArgs = @("VTOYCLI", "/I", "/Drive:$Drive", "/GPT", "/FS:exFAT")
& $VentoyExe @cliArgs
if ($LASTEXITCODE -ne 0 -and $LASTEXITCODE -ne $null) {
    Write-Host "CLI bez /Y exit=$LASTEXITCODE — retry z /Y..."
    & $VentoyExe VTOYCLI /I /Drive:$Drive /GPT /FS:exFAT /Y
}
if ($LASTEXITCODE -ne 0 -and $LASTEXITCODE -ne $null) {
    Write-Host "CLI nieudane — otwieram GUI Ventoy. Wybierz dysk $($disk.FriendlyName) i Install, potem zamknij."
    Start-Process -FilePath $VentoyExe -Wait
}

# Odswiez litere — Ventoy zwykle montuje jako VENTOY
Start-Sleep -Seconds 5
$ventoyVol = Get-Volume | Where-Object { $_.FileSystemLabel -match 'VENTOY' } | Select-Object -First 1
if (-not $ventoyVol -or -not $ventoyVol.DriveLetter) {
    Write-Host "Nie wykryto volume VENTOY automatycznie. Podaj litere partycji Ventoy (np. E):"
    $letter = (Read-Host).Trim().TrimEnd(':')
    $VentoyRoot = "${letter}:"
} else {
    $VentoyRoot = "$($ventoyVol.DriveLetter):"
}
Write-Host "Ventoy root: $VentoyRoot"

# --- ISO ---
$IsoPath = Join-Path $CacheDir $IsoName
if (-not (Test-Path $IsoPath)) {
    Write-Host "Pobieranie $IsoName (duzy plik)..."
    curl.exe -L --fail -C - -o $IsoPath $IsoUrl
}
$IsoDest = Join-Path $VentoyRoot $IsoName
if (-not (Test-Path $IsoDest)) {
    Write-Host "Kopiowanie ISO na Ventoy..."
    Copy-Item -Path $IsoPath -Destination $IsoDest -Force
} else {
    Write-Host "ISO juz na Ventoy: $IsoDest"
}

# --- Skrypty ---
$InstDest = Join-Path $VentoyRoot "tvbox-installer"
$ScriptsDest = Join-Path $InstDest "scripts"
$ImagesDest = Join-Path $VentoyRoot "images"
New-Item -ItemType Directory -Force -Path $ScriptsDest, $ImagesDest | Out-Null
Copy-Item -Path (Join-Path $ScriptDir "*.sh") -Destination $ScriptsDest -Force
Copy-Item -Path (Join-Path $ScriptDir "README.md") -Destination $InstDest -Force

# Shortcut runner
$RunCapture = @'
#!/bin/bash
M=$(blkid -L VENTOY 2>/dev/null || blkid -L Ventoy 2>/dev/null || true)
sudo mkdir -p /mnt/TVBOXIMG
if [ -n "$M" ]; then sudo mount "$M" /mnt/TVBOXIMG; fi
sudo bash /mnt/TVBOXIMG/tvbox-installer/scripts/tvbox-clone-capture.sh
'@
$RunRestore = @'
#!/bin/bash
M=$(blkid -L VENTOY 2>/dev/null || blkid -L Ventoy 2>/dev/null || true)
sudo mkdir -p /mnt/TVBOXIMG
if [ -n "$M" ]; then sudo mount "$M" /mnt/TVBOXIMG; fi
sudo bash /mnt/TVBOXIMG/tvbox-installer/scripts/tvbox-clone-restore.sh
'@
Set-Content -Path (Join-Path $InstDest "run-capture.sh") -Value $RunCapture -NoNewline -Encoding utf8
Set-Content -Path (Join-Path $InstDest "run-restore.sh") -Value $RunRestore -NoNewline -Encoding utf8

Write-Host ""
Write-Host "=== make_usb OK ==="
Write-Host "Ventoy: $VentoyRoot"
Write-Host "ISO:    $IsoDest"
Write-Host "Scripts:$ScriptsDest"
Write-Host "Images: $ImagesDest"
Write-Host ""
Write-Host "Nastepnie: boot golden z USB -> Ubuntu live -> bash tvbox-installer/run-capture.sh"
