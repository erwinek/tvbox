#Requires -Version 5.1
# Wyswietla kamery dostepne przez ffmpeg (dshow na Windows).
$ErrorActionPreference = "Stop"

if (-not (Get-Command ffmpeg -ErrorAction SilentlyContinue)) {
    Write-Host "Brak ffmpeg w PATH. Uruchom: scripts\setup_windows.cmd"
    exit 1
}

Write-Host "Szukam kamer (DirectShow)..."
$output = cmd /c "ffmpeg -hide_banner -list_devices true -f dshow -i dummy 2>&1"
$found = @()
foreach ($line in $output -split "`n") {
    if ($line -match '\(video\)' -and $line -notmatch 'Alternative name') {
        if ($line -match '"([^"]+)"') {
            $found += $Matches[1]
        }
    }
}

if ($found.Count -eq 0) {
    Write-Warning "Nie znaleziono kamer. Sprawdz kable USB i uprawnienia kamery w Windows."
    Write-Host ""
    Write-Host "Pelne wyjscie ffmpeg:"
    Write-Host $output
    exit 0
}

Write-Host ""
Write-Host "Znalezione kamery ($($found.Count)):"
for ($i = 0; $i -lt $found.Count; $i++) {
    $mark = if ($i -eq 0) { " <- uzywana przez TVBox (pierwsza)" } else { "" }
    Write-Host "  [$($i + 1)] $($found[$i])$mark"
}
