#Requires -Version 5.1
<#
.SYNOPSIS
  Przesyla pgm.ino.bin na Wyse i wgrywa na ESP32 (esptool przez /dev/ttyUSB0).

.PARAMETER Firmware
  Sciezka do binarki (domyslnie: ..\matrix2\matrix\liv\liv\pgm\pgm.ino.bin).

.PARAMETER Host
  SSH host alias (domyslnie: wyse).

.PARAMETER NoFlash
  Tylko scp binarki + skryptu, bez esptool.
#>
param(
    [string]$Firmware = "",
    [string]$HostName = "wyse",
    [switch]$NoFlash
)

$ErrorActionPreference = "Stop"
$ScriptDir = $PSScriptRoot
$RepoRoot = Split-Path -Parent $ScriptDir

if (-not $Firmware) {
    $Firmware = Join-Path (Split-Path -Parent $RepoRoot) "matrix2\matrix\liv\liv\pgm\pgm.ino.bin"
}

$FlashScriptLocal = Join-Path $ScriptDir "flash_esp32_wyse.sh"
$RemoteData = "/home/boxer/tvbox/data"
$RemoteFwDir = "$RemoteData/firmware"
$RemoteBin = "$RemoteFwDir/pgm.ino.bin"
$RemoteFlash = "$RemoteFwDir/flash_esp32_wyse.sh"

if (-not (Test-Path -LiteralPath $Firmware)) {
    throw "Brak firmware: $Firmware"
}
if (-not (Test-Path -LiteralPath $FlashScriptLocal)) {
    throw "Brak skryptu: $FlashScriptLocal"
}

Write-Host "=== Push firmware ESP32 -> $HostName ==="
Write-Host "local:  $Firmware"
Write-Host "remote: ${HostName}:$RemoteBin"

ssh -o BatchMode=yes $HostName "mkdir -p '$RemoteFwDir'"
if ($LASTEXITCODE -ne 0) { throw "ssh mkdir failed ($LASTEXITCODE)" }

scp -o BatchMode=yes $Firmware "${HostName}:$RemoteBin"
if ($LASTEXITCODE -ne 0) { throw "scp firmware failed ($LASTEXITCODE)" }

scp -o BatchMode=yes $FlashScriptLocal "${HostName}:$RemoteFlash"
if ($LASTEXITCODE -ne 0) { throw "scp flash script failed ($LASTEXITCODE)" }

ssh -o BatchMode=yes $HostName "sed -i 's/\r$//' '$RemoteFlash' && chmod +x '$RemoteFlash'"
if ($LASTEXITCODE -ne 0) { throw "ssh chmod failed ($LASTEXITCODE)" }

if ($NoFlash) {
    Write-Host "=== Upload OK (-NoFlash) ==="
    exit 0
}

Write-Host "--- Flash on $HostName ---"
ssh -o BatchMode=yes -t $HostName "bash '$RemoteFlash'"
if ($LASTEXITCODE -ne 0) { throw "flash failed ($LASTEXITCODE)" }

Write-Host "=== Done ==="
