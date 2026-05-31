# tvbox

GUI dla silomierza boxer na RPI4/5. SDL2, UART z ESP32, lokalny ranking offline, zapis wideo uderzenia i synchronizacja z serwerem.

## Architektura

Aplikacja jest maszyna stanow (FSM). Kazdy stan to osobny ekran (`Screen`):

```
CHOINKA (Attract) --kredyt--> GAME_START (ModeSelect) --start--> MEASURE --wynik--> END_GAME
   ^                                                                                   |
   +------------------------------ brak kredytow / timeout ----------------------------+
```

Podzial modulow w `src/`:
- `core/` — `StateMachine`, `Screen` (interfejs), `GameState`, `Events` (InputEvent), `AppContext`, `InputQueue`, `Clock`.
- `screens/` — `AttractScreen` (CHOINKA), `ModeSelectScreen` (GAME_START), `MeasureScreen`, `EndGameScreen`.
- `game/` — `GameSession`, `GameMode` (data-driven z configu), `Credits`, `ScoreEngine`.
- `io/` — `InputQueue`, `KeyboardInput`, `SerialInput` + `Protocol` (UART), `SerialReader` (low-level).
- `ui/` — `Renderer` (wrapper SDL), `FontManager`, `TextureCache`, `FrameSequencePlayer`, `widgets/` (Header, ScrollBar, LeaderboardWidget).
- `media/` — `VideoCapture` (nagrywanie ffmpeg), `AudioPlayer` (SDL2_mixer).
- `store/`, `sync/`, `config/`, `util/` — baza, synchronizacja, konfiguracja, logi.

Dodanie nowego stanu = nowa klasa w `screens/` + rejestracja w `App::RegisterScreens`.

### Sterowanie (tryb dev / klawiatura)
- `C` — wrzucenie kredytu (Coin)
- strzalki `<- ->` — wybor trybu gry
- `ENTER` — start / zatwierdzenie
- `SPACE` — symulacja uderzenia (w stanie MEASURE)
- `BACKSPACE` — cofnij
- `ESC` — wyjscie
- `1` / `2` / `3` / `4` — symulacja UART: skok do stanu CHOINKA / GAME_START / MEASURE / END_GAME

## Wymagania (Raspberry Pi OS)
- SDL2, SDL2_ttf, SDL2_image, SDL2_mixer
- SQLite3
- libcurl
- libcamera + GStreamer

Przykład instalacji:
```
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config \
  libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev libsdl2-mixer-dev \
  libsqlite3-dev libcurl4-openssl-dev \
  libcamera-apps gstreamer1.0-tools gstreamer1.0-plugins-base gstreamer1.0-plugins-good
```

## Build
```
mkdir -p build
cd build
cmake ..
make -j4
```

## Development na Windows (laptop)

Na laptopie mozesz developowac UI i logike aplikacji; finalny target to nadal RPI4/5.

### Szybki start (MSYS2 / MinGW)
```cmd
scripts\setup_windows.cmd   :: instaluje MSYS2, zaleznosci, buduje
scripts\run_windows.cmd     :: okno 1280x720, config\app-windows.yaml
```

Jesli PowerShell blokuje `.ps1` (Execution Policy), uzyj plikow `.cmd` powyzej.
Alternatywnie: `powershell -ExecutionPolicy Bypass -File .\scripts\run_windows.ps1`

Sterowanie w trybie dev:
- `SPACE` — symulacja uderzenia (score + ranking)
- `ESC` — wyjscie

UART (`COM3` domyslnie) i kamera sa opcjonalne — aplikacja startuje bez nich.
Serial mozesz podlaczyc przez USB-UART (ESP32); port ustaw w `config/app-windows.yaml`.

**Blad `nanosleep could not be located`:** uruchamiaj przez `scripts\run_windows.cmd` (kopiuje DLL MinGW obok exe). Nie uruchamiaj `tvbox_gui.exe` bezposrednio z Eksploratora.

Alternatywnie MSVC + vcpkg:
```powershell
git clone https://github.com/microsoft/vcpkg $env:USERPROFILE\vcpkg
.\vcpkg\bootstrap-vcpkg.bat
cmake -B build-msvc -S . -DCMAKE_TOOLCHAIN_FILE=$env:USERPROFILE\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build-msvc --config Debug
.\build-msvc\Debug\tvbox_gui.exe config\app-windows.yaml
```

## Uruchomienie (bez XServera, KMS/DRM)
```
SDL_VIDEODRIVER=kmsdrm SDL_RENDER_DRIVER=opengles2 SDL_FBDEV=/dev/fb0 \
./build/tvbox_gui config/app.yaml
```

Na RPI warto dodać użytkownika do grup `video` i `dialout`, a także upewnić się, że konsola ma dostęp do `/dev/dri` i `/dev/ttyUSB*`.

## Protokół UART (ESP32 -> RPI)
- Kredyt: `COIN` lub `CREDIT`
- Wynik (uderzenie): `SCORE,<value>,<playerId>,<unix_ms>`
- Stan: `STATE,HIT` (symulacja uderzenia)

Linie sa parsowane w `[src/io/Protocol.cpp](src/io/Protocol.cpp)` na zdarzenia `InputEvent`.

## Konfiguracja
Ustawienia w `config/app.yaml` (RPI) i `config/app-windows.yaml` (dev):
- `serial_port`, `baud_rate`
- `camera_command` (ffmpeg/libcamera, z `{duration_ms}` i `{output}`)
- `server_url`, `auth_token` (dla sync)
- `game_modes` — lista trybow `id:nazwa:mnoznik`, np. `"boxer:BOXER:1.0, kopacz:KOPACZ:1.1"`
- `sound_coin`, `sound_hit`, `sound_select`, `sound_win`, `music_attract` — opcjonalne audio (puste = cisza)

## Deployment na RPI5

### Szybki deploy (skrypt)
```bash
git clone <repo-url> ~/tvbox-src
cd ~/tvbox-src
chmod +x scripts/deploy_rpi5.sh
./scripts/deploy_rpi5.sh
```

Skrypt automatycznie:
1. Instaluje zależności systemowe
2. Buduje projekt (Release)
3. Kopiuje binarkę, config i font do `/home/erwinek/tvbox/`
4. Dodaje użytkownika do grup `video`, `dialout`, `render`, `input`
5. Instaluje i uruchamia usługę systemd `tvbox`

### Deploy przez CMake target
```bash
cd ~/tvbox-src/build
cmake .. -DCMAKE_BUILD_TYPE=Release
sudo make rpi5
sudo systemctl restart tvbox
```

### Zarządzanie usługą
```bash
sudo systemctl status tvbox
sudo systemctl stop tvbox
sudo systemctl restart tvbox
sudo journalctl -u tvbox -f
```

### Struktura po instalacji
```
/home/erwinek/tvbox/
├── bin/tvbox_gui
├── config/app.yaml
├── data/
│   ├── leaderboard.db
│   └── videos/
└── assets/
    └── fonts/DejaVuSans.ttf
```

## Uwagi
- Wymagany font pod `assets/fonts/DejaVuSans.ttf` lub podmień `font_path`.
- Dla UI grafik użyj `assets/` i komendy `UI,IMAGE`.
- Po zmianie `config/app.yaml` zrestartuj usługę: `sudo systemctl restart tvbox`.
