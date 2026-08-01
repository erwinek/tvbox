#Requires -Version 5.1
<#
.SYNOPSIS
  Deploy TVBox na Wyse z PC (sync + build on-target) i opcjonalnie flash ESP32.

.PARAMETER HostName
  SSH host alias (domyslnie: wyse).

.PARAMETER FlashEsp
  Po deploy GUI wgraj tez pgm.ino.bin na ESP32.

.PARAMETER SkipApp
  Pomin sync/build TVBox — tylko flash ESP32 (wymaga -FlashEsp).

.PARAMETER Firmware
  Sciezka do pgm.ino.bin (przekazywana do push_firmware_wyse.ps1).
#>
param(
    [string]$HostName = "wyse",
    [switch]$FlashEsp,
    [switch]$SkipApp,
    [string]$Firmware = ""
)

$ErrorActionPreference = "Stop"
$ScriptDir = $PSScriptRoot
$RepoRoot = Split-Path -Parent $ScriptDir
$RemoteSrc = "/home/boxer/tvbox/data/src/tvbox"
$PushFw = Join-Path $ScriptDir "push_firmware_wyse.ps1"

if ($SkipApp -and -not $FlashEsp) {
    throw "Uzyj -SkipApp razem z -FlashEsp (albo samo .\push_firmware_wyse.ps1)."
}

if (-not $SkipApp) {
    Write-Host "=== Deploy TVBox -> $HostName ==="
    Write-Host "local:  $RepoRoot"
    Write-Host "remote: ${HostName}:$RemoteSrc"

    ssh -o BatchMode=yes $HostName "mkdir -p '$RemoteSrc'"
    if ($LASTEXITCODE -ne 0) { throw "ssh mkdir failed ($LASTEXITCODE)" }

    $TarName = "tvbox-deploy.tgz"
    $TarLocal = Join-Path $env:TEMP $TarName
    if (Test-Path $TarLocal) { Remove-Item -Force $TarLocal }

    Push-Location $RepoRoot
    try {
        # Windows tar (bsdtar) — wyklucz build i ciezkie artefakty
        & tar.exe -czf $TarLocal `
            --exclude=build `
            --exclude=.git `
            --exclude=data/videos `
            --exclude=data/*.db `
            --exclude=data/*.log `
            .
        if ($LASTEXITCODE -ne 0) { throw "tar failed ($LASTEXITCODE)" }
    } finally {
        Pop-Location
    }

    scp -o BatchMode=yes $TarLocal "${HostName}:/tmp/$TarName"
    if ($LASTEXITCODE -ne 0) { throw "scp archive failed ($LASTEXITCODE)" }
    Remove-Item -Force $TarLocal -ErrorAction SilentlyContinue

    # Jedna linia bash — unikamy problemow z quoting PowerShell
    $RemoteCmd = "set -euo pipefail; mkdir -p '$RemoteSrc'; tar -xzf /tmp/$TarName -C '$RemoteSrc'; rm -f /tmp/$TarName; find '$RemoteSrc' -type f -name '*.sh' -exec sed -i 's/\r`$//' {} +; cd '$RemoteSrc'; bash scripts/deploy_wyse.sh"
    ssh -o BatchMode=yes -t $HostName $RemoteCmd
    if ($LASTEXITCODE -ne 0) { throw "remote deploy failed ($LASTEXITCODE)" }

    Write-Host "=== TVBox deploy OK ==="
}

if ($FlashEsp) {
    $fwArgs = @{ HostName = $HostName }
    if ($Firmware) { $fwArgs.Firmware = $Firmware }
    & $PushFw @fwArgs
    if ($LASTEXITCODE -ne 0) { throw "firmware push/flash failed ($LASTEXITCODE)" }
}

Write-Host "=== All done ==="
