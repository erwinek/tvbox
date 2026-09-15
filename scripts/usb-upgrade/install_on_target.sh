#!/usr/bin/env bash
# Wgraj watcher USB-upgrade na Wyse (live overlay + lowerdir).
set -euo pipefail

if [[ "$(id -u)" -ne 0 ]]; then
  echo "Uruchom: sudo bash $0"
  exit 1
fi

SRC="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
sed -i 's/\r$//' "$SRC"/*.sh "$SRC"/*.rules "$SRC"/*.service 2>/dev/null || true
chmod +x "$SRC/tvbox-usb-upgrade.sh" "$SRC/make_upgrade_usb.sh"

install_into() {
  local prefix="$1"
  install -D -m 0755 "$SRC/tvbox-usb-upgrade.sh" "${prefix}/usr/local/sbin/tvbox-usb-upgrade.sh"
  install -D -m 0644 "$SRC/tvbox-usb-upgrade@.service" "${prefix}/etc/systemd/system/tvbox-usb-upgrade@.service"
  install -D -m 0644 "$SRC/99-tvbox-usb-upgrade.rules" "${prefix}/etc/udev/rules.d/99-tvbox-usb-upgrade.rules"
}

echo "=== kopia na RW data ==="
install -D -m 0755 "$SRC/tvbox-usb-upgrade.sh" /home/boxer/tvbox/data/bin/tvbox-usb-upgrade.sh
mkdir -p /home/boxer/tvbox/data/usb-upgrade-src
cp -a "$SRC/tvbox-usb-upgrade.sh" "$SRC/tvbox-usb-upgrade@.service" \
  "$SRC/99-tvbox-usb-upgrade.rules" /home/boxer/tvbox/data/usb-upgrade-src/

echo "=== live overlay ==="
install_into ""
udevadm control --reload-rules
udevadm trigger --subsystem-match=block || true
systemctl daemon-reload

persist() {
  if [[ -d /run/rootfsbase ]]; then
    echo "=== persist /run/rootfsbase (lowerdir) ==="
    mount -o remount,rw /run/rootfsbase
    install_into /run/rootfsbase
    sync
    mount -o remount,ro /run/rootfsbase || true
    echo "lowerdir OK"
    return 0
  fi
  if command -v overlayroot-chroot >/dev/null && findmnt -n / | grep -q overlay; then
    echo "=== overlayroot-chroot ==="
    overlayroot-chroot /bin/bash -c "
      set -e
      cat > /usr/local/sbin/tvbox-usb-upgrade.sh
      chmod 0755 /usr/local/sbin/tvbox-usb-upgrade.sh
    " < "$SRC/tvbox-usb-upgrade.sh"
    overlayroot-chroot /bin/bash -c "cat > /etc/systemd/system/tvbox-usb-upgrade@.service" \
      < "$SRC/tvbox-usb-upgrade@.service"
    overlayroot-chroot /bin/bash -c "cat > /etc/udev/rules.d/99-tvbox-usb-upgrade.rules" \
      < "$SRC/99-tvbox-usb-upgrade.rules"
    echo "lowerdir OK"
    return 0
  fi
  echo "WARNING: nie zapisano lowerdir — watcher moze zniknac po reboot"
  return 1
}

persist

echo "=== Done ==="
echo "Log: /home/boxer/tvbox/data/usb-upgrade.log"
