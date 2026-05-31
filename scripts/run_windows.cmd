@echo off
setlocal
set "ROOT=%~dp0.."

if not exist "%ROOT%\build-mingw\tvbox_gui.exe" (
    echo Brak binarki. Uruchom najpierw: scripts\setup_windows.cmd
    exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0copy_mingw_dlls.ps1"
set "PATH=C:\msys64\mingw64\bin;%PATH%"

cd /d "%ROOT%"
echo TVBox dev (Windows) - ESC = wyjscie, SPACE = symulacja uderzenia
"%ROOT%\build-mingw\tvbox_gui.exe" "%ROOT%\config\app-windows.yaml"
