#Requires -Version 5.1
$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Exe = Join-Path $Root "build-mingw\tvbox_gui.exe"
$Config = Join-Path $Root "config\app-windows.yaml"

if (-not (Test-Path $Exe)) {
    Write-Host "Brak binarki. Uruchom najpierw: .\scripts\setup_windows.ps1"
    exit 1
}

Push-Location $Root
try {
    Write-Host "TVBox dev (Windows) - ESC = wyjscie, SPACE = symulacja uderzenia"
    & $Exe $Config
} finally {
    Pop-Location
}
