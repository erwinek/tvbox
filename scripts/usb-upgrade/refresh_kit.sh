#!/usr/bin/env bash
set -euo pipefail
STAMP="$(date -u -Iseconds)"
printf 'version=1.1.4\nupdated=%s\n' "$STAMP" | sudo tee /home/boxer/tvbox/data/app/current/VERSION >/dev/null
sudo mkdir -p /home/boxer/tvbox/data/upgrade-kit/TVBOX_UPDATE/bin \
  /home/boxer/tvbox/data/upgrade-kit/TVBOX_UPDATE/config
sudo cp -a /home/boxer/tvbox/data/app/current/bin/tvbox_gui \
  /home/boxer/tvbox/data/upgrade-kit/TVBOX_UPDATE/bin/
sudo cp -a /home/boxer/tvbox/config/app-wyse.yaml \
  /home/boxer/tvbox/data/upgrade-kit/TVBOX_UPDATE/config/
sudo cp -a /home/boxer/tvbox/data/app/current/VERSION \
  /home/boxer/tvbox/data/upgrade-kit/TVBOX_UPDATE/VERSION
sudo chown -R boxer:boxer /home/boxer/tvbox/data/upgrade-kit \
  /home/boxer/tvbox/data/app/current/VERSION
echo '--- VERSION ---'
cat /home/boxer/tvbox/data/upgrade-kit/TVBOX_UPDATE/VERSION
echo '--- yaml post_hit ---'
grep camera_post_hit /home/boxer/tvbox/data/upgrade-kit/TVBOX_UPDATE/config/app-wyse.yaml
echo '--- log ---'
grep -a 'TVBOX ' /home/boxer/tvbox/data/tvbox.log | tail -n 3
