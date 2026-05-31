@echo off
setlocal
set "ROOT=%~dp0.."
set "PATH=C:\msys64\mingw64\bin;%PATH%"
for /f "delims=" %%P in ('powershell -NoProfile -Command "[Environment]::GetEnvironmentVariable('Path','Machine') + ';' + [Environment]::GetEnvironmentVariable('Path','User')"') do set "PATH=%%P;%PATH%"

if not exist "%ROOT%\build-mingw\tvbox_gui.exe" (
    echo Brak binarki. Uruchom najpierw: scripts\setup_windows.cmd
    exit /b 1
)

cd /d "%ROOT%"
echo TVBox dev (Windows) - ESC = wyjscie, SPACE = symulacja uderzenia
"%ROOT%\build-mingw\tvbox_gui.exe" "%ROOT%\config\app-windows.yaml"
