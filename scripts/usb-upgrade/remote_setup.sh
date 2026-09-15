#!/usr/bin/env bash
set -euo pipefail
sudo mkdir -p /tmp/tvbox-usb-upgrade
sudo tar -xzf /tmp/tvbox-usb-upgrade.tgz -C /tmp/tvbox-usb-upgrade
sudo find /tmp/tvbox-usb-upgrade -type f -exec sed -i 's/\r$//' {} +
sudo chmod +x /tmp/tvbox-usb-upgrade/*.sh
sudo bash /tmp/tvbox-usb-upgrade/install_on_target.sh

STAMP="$(date -u -Iseconds)"
printf 'version=1.1.3\nupdated=%s\n' "$STAMP" | sudo tee /home/boxer/tvbox/data/app/current/VERSION >/dev/null

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

echo '--- persist check ---'
sudo ls -l /run/rootfsbase/usr/local/sbin/tvbox-usb-upgrade.sh \
  /run/rootfsbase/etc/udev/rules.d/99-tvbox-usb-upgrade.rules \
  /usr/local/sbin/tvbox-usb-upgrade.sh
echo '--- kit ---'
cat /home/boxer/tvbox/data/upgrade-kit/TVBOX_UPDATE/VERSION
ls -l /home/boxer/tvbox/data/upgrade-kit/TVBOX_UPDATE/bin
echo '--- usb ---'
lsblk -o NAME,TRAN,RM,SIZE,LABEL
sudo bash /tmp/tvbox-usb-upgrade/make_upgrade_usb.sh || true
findmnt -n /run/rootfsbase || true
