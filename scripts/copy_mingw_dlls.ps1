#Requires -Version 5.1
# Kopiuje DLL MinGW obok tvbox_gui.exe (naprawia: nanosleep could not be located...)
$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "build-mingw"
$MsysBin = "C:\msys64\mingw64\bin"

if (-not (Test-Path $MsysBin)) {
    Write-Warning "Brak $MsysBin - pominieto kopiowanie DLL"
    exit 0
}

$Dlls = @(
    "libwinpthread-1.dll",
    "SDL2.dll",
    "SDL2_ttf.dll",
    "SDL2_image.dll",
    "SDL2_mixer.dll",
    "libcurl-4.dll",
    "libsqlite3-0.dll",
    "zlib1.dll",
    "libpng16-16.dll",
    "libjpeg-8.dll",
    "libwebp-7.dll",
    "libtiff-6.dll",
    "libfreetype-6.dll",
    "libharfbuzz-0.dll",
    "libbrotlidec.dll",
    "libbrotlicommon.dll",
    "libbz2-1.dll",
    "libstdc++-6.dll",
    "libgcc_s_seh-1.dll",
    "libogg-0.dll",
    "libvorbis-0.dll",
    "libvorbisfile-3.dll",
    "libmpg123-0.dll",
    "libopus-0.dll",
    "libopusfile-0.dll",
    "libFLAC.dll",
    "libmodplug-1.dll",
    "libidn2-0.dll",
    "libunistring-5.dll",
    "libnghttp2-14.dll",
    "libnghttp3-9.dll",
    "libpsl-5.dll",
    "libssh2-1.dll",
    "libssl-3-x64.dll",
    "libcrypto-3-x64.dll",
    "libzstd.dll",
    "libb2-1.dll",
    "libgraphite2.dll",
    "libiconv-2.dll",
    "libintl-8.dll",
    "libpcre2-8-0.dll",
    "libdeflate.dll",
    "libjbig-0.dll",
    "libLerc.dll",
    "liblzma-5.dll",
    "libsharpyuv-0.dll"
)

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
$copied = 0
foreach ($dll in $Dlls) {
    $src = Join-Path $MsysBin $dll
    if (Test-Path $src) {
        Copy-Item -Force $src (Join-Path $BuildDir $dll)
        $copied++
    }
}
Write-Host "Skopiowano $copied DLL do $BuildDir"
