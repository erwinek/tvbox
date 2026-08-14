#!/bin/bash
set -euo pipefail
[[ "$(id -u)" -eq 0 ]] || { echo "sudo $0"; exit 1; }

LOWER=/run/rootfsbase
if [[ -d "$LOWER/etc" ]]; then
  mount -o remount,rw "$LOWER" || true
fi

hide_grub() {
  local f="$1/etc/default/grub"
  [[ -f "$f" ]] || return 0
  sed -i "s/^GRUB_TIMEOUT=.*/GRUB_TIMEOUT=0/" "$f"
  sed -i "s/^GRUB_TIMEOUT_STYLE=.*/GRUB_TIMEOUT_STYLE=hidden/" "$f"
  grep -q "^GRUB_TIMEOUT=" "$f" || echo "GRUB_TIMEOUT=0" >>"$f"
  grep -q "^GRUB_TIMEOUT_STYLE=" "$f" || echo "GRUB_TIMEOUT_STYLE=hidden" >>"$f"
}

install_wait() {
  local root="$1"
  mkdir -p "$root/usr/local/sbin" "$root/etc/systemd/system/tvbox.service.d"
  cat >"$root/usr/local/sbin/tvbox-wait-camera.sh" <<'EOS'
#!/bin/bash
for _ in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
  if [ -e /dev/video0 ]; then
    exit 0
  fi
  sleep 1
done
exit 0
EOS
  chmod +x "$root/usr/local/sbin/tvbox-wait-camera.sh"
  cat >"$root/etc/systemd/system/tvbox.service.d/wait-camera.conf" <<'EOF'
[Service]
ExecStartPre=/usr/local/sbin/tvbox-wait-camera.sh
EOF
}

hide_grub /
install_wait /
if [[ -d "$LOWER/etc" ]]; then
  hide_grub "$LOWER"
  install_wait "$LOWER"
  mount -o remount,ro "$LOWER" || true
fi

# overlayroot: update-grub fails (LiveOS_rootfs). Patch grub.cfg on /boot (RW).
if [[ -f /boot/grub/grub.cfg ]]; then
  sed -i 's/^set timeout=.*/set timeout=0/' /boot/grub/grub.cfg
  sed -i 's/^set timeout_style=.*/set timeout_style=hidden/' /boot/grub/grub.cfg
  # recordfail would force menu — keep timeout 0
  sed -i 's/if \[ \${recordfail} = 1 \]; then set timeout=.*/if [ ${recordfail} = 1 ]; then set timeout=0/' /boot/grub/grub.cfg || true
fi

systemctl daemon-reload
systemctl restart tvbox.service
sleep 4
echo "ACTIVE=$(systemctl is-active tvbox.service)"
echo "=== grub.cfg timeout ==="
grep -E "set timeout" /boot/grub/grub.cfg | head -8
echo "=== VideoCapture ==="
grep -a "VideoCapture" /home/boxer/tvbox/data/tvbox.log | tail -10
