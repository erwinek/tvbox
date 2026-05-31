#Requires -Version 5.1
$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot

function Ensure-Msys2 {
    $MsysRoot = "C:\msys64"
    if (-not (Test-Path "$MsysRoot\usr\bin\bash.exe")) {
        Write-Host "Instalacja MSYS2 (MinGW64)..."
        winget install --id MSYS2.MSYS2 -e --accept-package-agreements --accept-source-agreements
        if (-not (Test-Path "$MsysRoot\usr\bin\bash.exe")) {
            throw "MSYS2 nie zostal zainstalowany w $MsysRoot"
        }
    }
    return $MsysRoot
}

function Ensure-Ffmpeg {
    if (Get-Command ffmpeg -ErrorAction SilentlyContinue) {
        $src = (Get-Command ffmpeg).Source
        Write-Host "ffmpeg juz dostepny: $src"
        return
    }

    Write-Host "Instalacja ffmpeg (winget, wymagane do nagrywania z kamerki)..."
    winget install --id Gyan.FFmpeg -e --accept-package-agreements --accept-source-agreements

    $env:PATH = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" +
                [System.Environment]::GetEnvironmentVariable("Path", "User")

    if (Get-Command ffmpeg -ErrorAction SilentlyContinue) {
        Write-Host "ffmpeg zainstalowany: $((Get-Command ffmpeg).Source)"
        Write-Host ""
        & (Join-Path $PSScriptRoot "detect_camera.ps1")
        return
    }

    Write-Warning "ffmpeg moze byc zainstalowany, ale nie widoczny w PATH tej sesji."
    Write-Warning "Zamknij terminal, otworz nowy i sprawdz: ffmpeg -version"
    Write-Warning "Liste kamer: ffmpeg -list_devices true -f dshow -i dummy"
}

function Ensure-Font {
    $FontDir = Join-Path $Root "assets\fonts"
    $FontPath = Join-Path $FontDir "DejaVuSans.ttf"
    if (-not (Test-Path $FontPath)) {
        New-Item -ItemType Directory -Force -Path $FontDir | Out-Null
        $ZipUrl = "https://downloads.sourceforge.net/project/dejavu/dejavu/2.37/dejavu-fonts-ttf-2.37.zip"
        $ZipPath = Join-Path $env:TEMP "dejavu-fonts-ttf-2.37.zip"
        Write-Host "Pobieranie fontu DejaVuSans.ttf..."
        & curl.exe -L -o $ZipPath $ZipUrl
        Expand-Archive -Path $ZipPath -DestinationPath (Join-Path $env:TEMP "dejavu-fonts") -Force
        Copy-Item (Join-Path $env:TEMP "dejavu-fonts\dejavu-fonts-ttf-2.37\ttf\DejaVuSans.ttf") $FontPath
    }
}

function Install-Deps {
    param([string]$MsysRoot)
    $Packages = @(
        "mingw-w64-x86_64-gcc",
        "mingw-w64-x86_64-cmake",
        "mingw-w64-x86_64-ninja",
        "mingw-w64-x86_64-pkgconf",
        "mingw-w64-x86_64-SDL2",
        "mingw-w64-x86_64-SDL2_ttf",
        "mingw-w64-x86_64-SDL2_image",
        "mingw-w64-x86_64-SDL2_mixer",
        "mingw-w64-x86_64-sqlite3",
        "mingw-w64-x86_64-curl"
    )
    $PackageList = ($Packages -join " ")
    Write-Host "Instalacja zaleznosci MSYS2..."
    $Bash = Join-Path $MsysRoot "usr\bin\bash.exe"
    & $Bash -lc "pacman -Syy --noconfirm --needed $PackageList"
}

function Build-Project {
    param([string]$MsysRoot)
    $BuildDir = Join-Path $Root "build-mingw"
    $RootPosix = ($Root -replace '\\', '/')
    $BuildPosix = ($BuildDir -replace '\\', '/')
    Write-Host "Budowanie w $BuildDir ..."
    $Bash = Join-Path $MsysRoot "usr\bin\bash.exe"
    & $Bash -lc @"
export PATH=/mingw64/bin:/usr/bin:`$PATH
mkdir -p '$BuildPosix'
cd '$BuildPosix'
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug '$RootPosix'
cmake --build .
"@
}

Ensure-Font
Ensure-Ffmpeg
$Msys = Ensure-Msys2
Install-Deps -MsysRoot $Msys
Build-Project -MsysRoot $Msys
& (Join-Path $PSScriptRoot "copy_mingw_dlls.ps1")

Write-Host ""
Write-Host "Gotowe. Uruchom: .\scripts\run_windows.ps1"
