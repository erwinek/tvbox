#!/usr/bin/env bash
set -euo pipefail
cd /home/boxer/tvbox
export SDL_VIDEODRIVER=wayland
# Logi do pliku (dziecko sway nie idzie do journal systemd).
exec >>/home/boxer/tvbox/data/tvbox.log 2>&1
echo "=== start $(date -Is) WAYLAND_DISPLAY=${WAYLAND_DISPLAY:-} ==="
./bin/tvbox_gui ./config/app-wyse.yaml || true
echo "=== exit $? $(date -Is) ==="
swaymsg exit 2>/dev/null || true
