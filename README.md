# tvbox

GUI dla silomierza boxer na RPI4/5. SDL2, UART z ESP32, lokalny ranking offline, zapis wideo uderzenia i synchronizacja z serwerem.

## Wymagania (Raspberry Pi OS)
- SDL2, SDL2_ttf, SDL2_image
- SQLite3
- libcurl
- libcamera + GStreamer

Przykład instalacji:
```
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config \
  libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev \
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

## Uruchomienie (bez XServera, KMS/DRM)
```
SDL_VIDEODRIVER=kmsdrm SDL_RENDER_DRIVER=opengles2 SDL_FBDEV=/dev/fb0 \
./build/tvbox_gui config/app.yaml
```

Na RPI warto dodać użytkownika do grup `video` i `dialout`, a także upewnić się, że konsola ma dostęp do `/dev/dri` i `/dev/ttyUSB*`.

## Protokół UART (ESP32 -> RPI)
- Wynik: `SCORE,<value>,<playerId>,<unix_ms>`
- Stan: `STATE,IDLE` / `STATE,READY` / `STATE,HIT`
- UI: `UI,SCENE,<name>` / `UI,TEXT,<id>,<value>` / `UI,IMAGE,<id>,<assetKey>`

## Konfiguracja
Ustawienia w `config/app.yaml`:
- `serial_port`, `baud_rate`
- `camera_command` (libcamera + GStreamer, z `{duration_ms}` i `{output}`)
- `server_url`, `auth_token` (dla sync)

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
