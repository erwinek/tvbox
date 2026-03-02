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

## Uwagi
- Wymagany font pod `assets/fonts/DejaVuSans.ttf` lub podmień `font_path`.
- Dla UI grafik użyj `assets/` i komendy `UI,IMAGE`.
