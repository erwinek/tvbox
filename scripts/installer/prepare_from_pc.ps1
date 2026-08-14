#Requires -Version 5.1
<#
.SYNOPSIS
  Z PC: wgrywa skrypty na golden i odpala prepare USB.
  USB musi byc wlozony w golden Wyse.
#>
param(
    [string]$HostName = "wyse"
)

$ErrorActionPreference = "Stop"
$ScriptDir = $PSScriptRoot
$Remote = "/tmp/tvbox-installer"
$tar = Join-Path $env:TEMP "tvbox-installer-push.tgz"

Write-Host "=== Pack installer ==="
if (Test-Path $tar) { Remove-Item -Force $tar }
Push-Location $ScriptDir
& tar.exe -czf $tar `
  --exclude=make_usb.ps1 `
  --exclude=make_usb.sh `
  --exclude=prepare_from_pc.ps1 `
  --exclude=write_ubuntu_usb.ps1 `
  --exclude=write_ubuntu_usb_on_golden.sh `
  --exclude=fix_usb_tvboximg_on_golden.sh `
  --exclude=_fix_tvboximg.sh `
  *
Pop-Location

Write-Host "=== Upload -> $HostName ==="
scp -o BatchMode=yes $tar "${HostName}:/tmp/tvbox-installer-push.tgz"
if ($LASTEXITCODE -ne 0) { throw "scp failed - check ssh $HostName and USB in golden" }

Write-Host "=== Capture na USB (sudo na golden) ==="
ssh -o BatchMode=yes -t $HostName "set -euo pipefail; rm -rf /tmp/tvbox-installer; mkdir -p /tmp/tvbox-installer; tar -xzf /tmp/tvbox-installer-push.tgz -C /tmp/tvbox-installer; find /tmp/tvbox-installer -type f -name '*.sh' -exec sed -i 's/\r`$//' {} +; find /tmp/tvbox-installer -type f -name '*.sh' -exec chmod +x {} +; test -f /tmp/tvbox-installer/common.sh; test -f /tmp/tvbox-installer/tvbox-usb-prepare-golden.sh; sudo bash /tmp/tvbox-installer/tvbox-usb-prepare-golden.sh"
if ($LASTEXITCODE -ne 0) { throw "prepare on golden failed ($LASTEXITCODE)" }

Write-Host ""
Write-Host "=== OK ==="
Write-Host "1. Wyjmij USB z golden"
Write-Host "2. Wloz do target 16GB, boot z USB -> Ubuntu"
Write-Host "3. Auto-restore albo: sudo bash .../tvbox-auto-restore.sh"
