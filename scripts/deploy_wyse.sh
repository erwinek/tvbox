#!/usr/bin/env bash
set -euo pipefail

INSTALL_DIR="/home/boxer/tvbox"
SERVICE_NAME="tvbox"
SERVICE_USER="boxer"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "=== TVBox Wyse Deployment ==="
echo "Install dir: $INSTALL_DIR"
echo "Source dir:   $PROJECT_DIR"
echo "User:         $SERVICE_USER"

echo ""
echo "--- [1/7] Installing dependencies ---"
sudo apt-get update -qq
sudo apt-get install -y --no-install-recommends \
  build-essential cmake pkg-config \
  libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev libsdl2-mixer-dev \
  libsqlite3-dev libcurl4-openssl-dev \
  ffmpeg \
  fonts-dejavu-core

echo ""
echo "--- [2/7] Building project ---"
mkdir -p "$PROJECT_DIR/build"
cd "$PROJECT_DIR/build"
# Wyse Celeron = x86-64-v2 (bez AVX); unikamy note ISA v3 z nowszych toolchainow.
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-march=x86-64-v2 -mtune=generic"
make -j"$(nproc)"
# Usun note.gnu.property jesli linker nadal oznaczyl v3 (glibc odrzuca binarke).
if command -v objcopy >/dev/null 2>&1; then
  objcopy --remove-section=.note.gnu.property tvbox_gui tvbox_gui.stripped 2>/dev/null \
    && mv tvbox_gui.stripped tvbox_gui \
    || true
fi
cd "$PROJECT_DIR"

echo ""
echo "--- [3/7] Creating install directories ---"
mkdir -p "$INSTALL_DIR/bin"
mkdir -p "$INSTALL_DIR/config"
mkdir -p "$INSTALL_DIR/data/videos"
mkdir -p "$INSTALL_DIR/assets/fonts"
mkdir -p "$INSTALL_DIR/assets/backgrounds"

echo ""
echo "--- [4/7] Installing files ---"
cp "$PROJECT_DIR/build/tvbox_gui" "$INSTALL_DIR/bin/tvbox_gui"

if [ ! -f "$INSTALL_DIR/config/app-wyse.yaml" ]; then
  cp "$PROJECT_DIR/config/app-wyse.yaml" "$INSTALL_DIR/config/app-wyse.yaml"
  echo "Installed default config (app-wyse.yaml)"
else
  echo "Config already exists, skipping (check for new options in source config)"
fi

FONT_SRC="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
FONT_DST="$INSTALL_DIR/assets/fonts/DejaVuSans.ttf"
if [ -f "$FONT_SRC" ] && [ ! -f "$FONT_DST" ]; then
  cp "$FONT_SRC" "$FONT_DST"
  echo "Installed font"
elif [ -f "$PROJECT_DIR/assets/fonts/DejaVuSans.ttf" ] && [ ! -f "$FONT_DST" ]; then
  cp "$PROJECT_DIR/assets/fonts/DejaVuSans.ttf" "$FONT_DST"
  echo "Installed font from project assets"
fi

echo ""
echo "--- [5/7] Setting permissions ---"
sudo usermod -aG video,dialout,render,input "$SERVICE_USER" 2>/dev/null || true

echo ""
echo "--- [6/7] Installing systemd service ---"
sudo cp "$PROJECT_DIR/scripts/tvbox-wyse.service" /etc/systemd/system/${SERVICE_NAME}.service
sudo systemctl daemon-reload
sudo systemctl enable ${SERVICE_NAME}.service
echo "Service ${SERVICE_NAME} enabled"

echo ""
echo "--- [7/7] Starting service ---"
sudo systemctl restart ${SERVICE_NAME}.service
sleep 2
sudo systemctl status ${SERVICE_NAME}.service --no-pager || true

echo ""
echo "=== Deployment complete ==="
echo "Logs:    sudo journalctl -u ${SERVICE_NAME} -f"
echo "Stop:    sudo systemctl stop ${SERVICE_NAME}"
echo "Restart: sudo systemctl restart ${SERVICE_NAME}"
