#!/usr/bin/env bash
# Deploy TVBox na Dell Wyse — Sway FHD (domyslnie). DRM atomic probe w bin/.
set -euo pipefail

INSTALL_DIR="/home/boxer/tvbox"
SERVICE_NAME="tvbox"
SERVICE_USER="boxer"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "=== TVBox Wyse Kiosk (Sway FHD) ==="

if [[ "$(id -u)" -eq 0 ]]; then
  echo "Nie uruchamiaj jako root."
  exit 1
fi

echo ""
echo "--- [1/7] Dependencies ---"
sudo apt-get update -qq
sudo apt-get install -y --no-install-recommends \
  build-essential cmake pkg-config \
  libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev libsdl2-mixer-dev \
  libgbm-dev libdrm-dev libegl1-mesa-dev \
  libsqlite3-dev libcurl4-openssl-dev \
  ffmpeg fonts-dejavu-core git \
  sway seatd

echo ""
echo "--- [2/7] Build ---"
mkdir -p "$PROJECT_DIR/build"
cd "$PROJECT_DIR/build"
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-march=x86-64-v2 -mtune=generic"
cmake --build . -j"$(nproc)"
if command -v objcopy >/dev/null 2>&1; then
  objcopy --remove-section=.note.gnu.property tvbox_gui tvbox_gui.stripped 2>/dev/null \
    && mv tvbox_gui.stripped tvbox_gui || true
fi
cd "$PROJECT_DIR"

echo ""
echo "--- [3/7] Install dirs ---"
mkdir -p "$INSTALL_DIR/bin" "$INSTALL_DIR/config" \
  "$INSTALL_DIR/data/videos" "$INSTALL_DIR/assets/fonts" "$INSTALL_DIR/assets/backgrounds"

echo ""
echo "--- [4/7] Install files ---"
# Zatrzymaj usluge zanim nadpiszemy binarke (cp: Text file busy).
sudo systemctl stop ${SERVICE_NAME}.service 2>/dev/null || true
sleep 1
cp "$PROJECT_DIR/build/tvbox_gui" "$INSTALL_DIR/bin/tvbox_gui"
chmod +x "$INSTALL_DIR/bin/tvbox_gui"
cp "$PROJECT_DIR/config/app-wyse.yaml" "$INSTALL_DIR/config/app-wyse.yaml"
cp "$PROJECT_DIR/config/sway-kiosk.conf" "$INSTALL_DIR/config/sway-kiosk.conf"
cp "$PROJECT_DIR/scripts/tvbox-kiosk-run.sh" "$INSTALL_DIR/bin/tvbox-kiosk-run.sh"
chmod +x "$INSTALL_DIR/bin/tvbox-kiosk-run.sh"
sed -i 's/\r$//' "$INSTALL_DIR/bin/tvbox-kiosk-run.sh" "$INSTALL_DIR/config/sway-kiosk.conf" || true
[ -f "$PROJECT_DIR/build/drm_rotate_probe" ] && cp "$PROJECT_DIR/build/drm_rotate_probe" "$INSTALL_DIR/bin/"
# Low-res klipy tla (ModeSelect) — bez tego ffmpeg dekoduje stare HD i zjada CPU.
if [[ -d "$PROJECT_DIR/assets/backgrounds" ]]; then
  mkdir -p "$INSTALL_DIR/assets/backgrounds"
  cp -f "$PROJECT_DIR/assets/backgrounds/"*.mp4 "$INSTALL_DIR/assets/backgrounds/" 2>/dev/null || true
fi

FONT_SRC="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
FONT_DST="$INSTALL_DIR/assets/fonts/DejaVuSans.ttf"
[ -f "$FONT_SRC" ] && cp "$FONT_SRC" "$FONT_DST"

echo ""
echo "--- [5/7] Groups / seatd ---"
sudo usermod -aG video,dialout,render,input "$SERVICE_USER"
sudo systemctl enable --now seatd.service 2>/dev/null || true

echo ""
echo "--- [6/7] systemd ---"
sudo cp "$PROJECT_DIR/scripts/tvbox-kiosk.service" /etc/systemd/system/${SERVICE_NAME}.service
sudo systemctl daemon-reload
sudo systemctl enable ${SERVICE_NAME}.service
sudo systemctl disable getty@tty1.service 2>/dev/null || true

echo ""
echo "--- [7/7] Start ---"
sudo systemctl restart ${SERVICE_NAME}.service
sleep 3
sudo systemctl status ${SERVICE_NAME}.service --no-pager || true

echo ""
echo "--- Hardening (power-cycle) ---"
if [[ -f "$PROJECT_DIR/scripts/harden_wyse.sh" ]]; then
  sed -i 's/\r$//' "$PROJECT_DIR/scripts/harden_wyse.sh" || true
  sudo bash "$PROJECT_DIR/scripts/harden_wyse.sh"
fi

echo ""
echo "=== Done ==="
echo "DRM atomic probe: sudo $INSTALL_DIR/bin/drm_rotate_probe 270 1920 1080"
echo "Logs: sudo journalctl -u ${SERVICE_NAME} -f / $INSTALL_DIR/data/tvbox.log"
echo "Hardening: scripts/harden_wyse.sh"
echo "Readonly root + RW data: sudo bash scripts/setup_readonly_root.sh && sudo reboot"
