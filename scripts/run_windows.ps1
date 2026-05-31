#Requires -Version 5.1
$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Exe = Join-Path $Root "build-mingw\tvbox_gui.exe"
$Config = Join-Path $Root "config\app-windows.yaml"
$MsysBin = "C:\msys64\mingw64\bin"

if (-not (Test-Path $Exe)) {
    Write-Host "Brak binarki. Uruchom najpierw: .\scripts\setup_windows.ps1"
    exit 1
}

# libwinpthread-1.dll (nanosleep) + SDL musza byc obok exe lub z MinGW PATH
& (Join-Path $PSScriptRoot "copy_mingw_dlls.ps1")
$env:PATH = "$MsysBin;$env:PATH"

Push-Location $Root
try {
    Write-Host "TVBox dev (Windows) - ESC = wyjscie, SPACE = symulacja uderzenia"
    & $Exe $Config
} finally {
    Pop-Location
}
