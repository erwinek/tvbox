#!/usr/bin/env bash
set -euo pipefail

# Prefer durable overlay-safe app tree; fall back to classic install path.
APP_ROOT="/home/boxer/tvbox/data/app/current"
if [[ ! -x "$APP_ROOT/bin/tvbox_gui" ]]; then
  APP_ROOT="/home/boxer/tvbox"
fi

cd "$APP_ROOT"
export SDL_VIDEODRIVER=wayland
mkdir -p /home/boxer/tvbox/data
# Logi do pliku (dziecko sway nie idzie do journal systemd).
exec >>/home/boxer/tvbox/data/tvbox.log 2>&1
echo "=== start $(date -Is) APP_ROOT=$APP_ROOT WAYLAND_DISPLAY=${WAYLAND_DISPLAY:-} ==="
CFG=./config/app-wyse.yaml
[[ -f "$CFG" ]] || CFG=/home/boxer/tvbox/config/app-wyse.yaml
./bin/tvbox_gui "$CFG" || true
echo "=== exit $? $(date -Is) ==="
swaymsg exit 2>/dev/null || true
