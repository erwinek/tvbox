#!/usr/bin/env bash
set -euo pipefail

INSTALL_DIR="/home/erwinek/tvbox"
SERVICE_NAME="tvbox"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "=== TVBox RPI5 Deployment ==="
echo "Install dir: $INSTALL_DIR"
echo "Source dir:   $PROJECT_DIR"

echo ""
echo "--- [1/7] Installing dependencies ---"
sudo apt-get update -qq
sudo apt-get install -y --no-install-recommends \
  build-essential cmake pkg-config \
  libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev \
  libsqlite3-dev libcurl4-openssl-dev \
  libcamera-apps gstreamer1.0-tools \
  gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
  fonts-dejavu-core

echo ""
echo "--- [2/7] Building project ---"
mkdir -p "$PROJECT_DIR/build"
cd "$PROJECT_DIR/build"
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j"$(nproc)"
cd "$PROJECT_DIR"

echo ""
echo "--- [3/7] Creating install directories ---"
mkdir -p "$INSTALL_DIR/bin"
mkdir -p "$INSTALL_DIR/config"
mkdir -p "$INSTALL_DIR/data/videos"
mkdir -p "$INSTALL_DIR/assets/fonts"

echo ""
echo "--- [4/7] Installing files ---"
cp "$PROJECT_DIR/build/tvbox_gui" "$INSTALL_DIR/bin/tvbox_gui"

if [ ! -f "$INSTALL_DIR/config/app.yaml" ]; then
  cp "$PROJECT_DIR/config/app.yaml" "$INSTALL_DIR/config/app.yaml"
  echo "Installed default config"
else
  echo "Config already exists, skipping (check for new options in source config)"
fi

FONT_SRC="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
FONT_DST="$INSTALL_DIR/assets/fonts/DejaVuSans.ttf"
if [ -f "$FONT_SRC" ] && [ ! -f "$FONT_DST" ]; then
  cp "$FONT_SRC" "$FONT_DST"
  echo "Installed font"
fi

echo ""
echo "--- [5/7] Setting permissions ---"
sudo usermod -aG video,dialout,render,input erwinek 2>/dev/null || true

echo ""
echo "--- [6/7] Installing systemd service ---"
sudo cp "$PROJECT_DIR/scripts/tvbox.service" /etc/systemd/system/${SERVICE_NAME}.service
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
