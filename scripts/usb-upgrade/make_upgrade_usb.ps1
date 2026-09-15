#Requires -Version 5.1
<#
.SYNOPSIS
  Kopiuje TVBOX_UPDATE na pendrive (FAT32/exFAT) podlaczony do tego PC.
#>
param(
    [string]$Drive = "",
    [string]$HostName = "wyse"
)

$ErrorActionPreference = "Stop"
$DestRoot = Join-Path $PSScriptRoot "..\..\dist\TVBOX_UPDATE"
$DestRoot = [IO.Path]::GetFullPath($DestRoot)

Write-Host "=== Pobieram kit z $HostName ==="
New-Item -ItemType Directory -Force -Path (Join-Path $DestRoot "bin"), (Join-Path $DestRoot "config") | Out-Null
scp -o BatchMode=yes "${HostName}:/home/boxer/tvbox/data/upgrade-kit/TVBOX_UPDATE/bin/tvbox_gui" (Join-Path $DestRoot "bin\tvbox_gui")
if ($LASTEXITCODE -ne 0) { throw "scp tvbox_gui failed" }
scp -o BatchMode=yes "${HostName}:/home/boxer/tvbox/data/upgrade-kit/TVBOX_UPDATE/VERSION" (Join-Path $DestRoot "VERSION")
scp -o BatchMode=yes "${HostName}:/home/boxer/tvbox/data/upgrade-kit/TVBOX_UPDATE/config/app-wyse.yaml" (Join-Path $DestRoot "config\app-wyse.yaml")

if (-not $Drive) {
    $vol = Get-Volume | Where-Object { $_.DriveType -eq 'Removable' -and $_.DriveLetter } | Select-Object -First 1
    if ($vol) { $Drive = "$($vol.DriveLetter):" }
}

if (-not $Drive) {
    Write-Host "Kit gotowy: $DestRoot"
    Write-Host "Wloz pendrive FAT32 i odpal ponownie, albo skopiuj folder TVBOX_UPDATE na korzen USB."
    exit 0
}

$Drive = $Drive.TrimEnd('\').TrimEnd(':') + ':'
$usbDest = Join-Path $Drive "TVBOX_UPDATE"
Write-Host "=== Zapis na $usbDest ==="
New-Item -ItemType Directory -Force -Path (Join-Path $usbDest "bin"), (Join-Path $usbDest "config") | Out-Null
Copy-Item (Join-Path $DestRoot "VERSION") (Join-Path $usbDest "VERSION") -Force
Copy-Item (Join-Path $DestRoot "bin\tvbox_gui") (Join-Path $usbDest "bin\tvbox_gui") -Force
Copy-Item (Join-Path $DestRoot "config\app-wyse.yaml") (Join-Path $usbDest "config\app-wyse.yaml") -Force
Write-Host "OK. Wyjmij pendrive."
Write-Host "U klienta: wloz w port USB Dell Wyse (thin client w szafce), nie w monitor i nie bootuj z USB."
