#!/usr/bin/env bash
# Setup installer appliance on USB Ubuntu (run once as root).
set -euo pipefail
SRC="${1:-/tmp/tvbox-installer-deploy}"
DST=/opt/tvbox-installer

apt-get update -qq
apt-get install -y --no-install-recommends whiptail rsync tar gzip lvm2 gdisk parted \
  e2fsprogs dosfstools grub-efi-amd64-bin efibootmgr ca-certificates

mkdir -p "$DST/scripts" "$DST/payload/app" "$DST/payload/os" "$DST/images"
# Replace scripts only — never wipe payload
find "$DST/scripts" -mindepth 1 -delete 2>/dev/null || true
cp -a "$SRC"/scripts/*.sh "$DST/scripts/" 2>/dev/null || cp -a "$SRC"/*.sh "$DST/scripts/"
# if tarball extracted flat
if [[ -f "$SRC/common.sh" ]]; then
  cp -a "$SRC"/*.sh "$DST/scripts/"
fi
chmod +x "$DST/scripts"/*.sh
sed -i 's/\r$//' "$DST/scripts"/*.sh

# migrate existing capture if present under images
if [[ -d "$SRC/payload" ]]; then
  cp -a "$SRC/payload"/* "$DST/payload/" 2>/dev/null || true
fi

ln -sfn "$DST/scripts/tvbox-installer-menu.sh" /usr/local/sbin/tvbox-menu
chmod +x "$DST/scripts/tvbox-installer-menu.sh"

# sudoers NOPASSWD for boxer (installer appliance)
echo 'boxer ALL=(ALL) NOPASSWD:ALL' >/etc/sudoers.d/90-boxer-installer
chmod 440 /etc/sudoers.d/90-boxer-installer

# Menu as systemd service on tty1 (getty drop-in is ignored on Ubuntu 24.04)
if [[ -f "$SRC/tvbox-installer-menu.service" ]]; then
  cp -a "$SRC/tvbox-installer-menu.service" /etc/systemd/system/tvbox-installer-menu.service
elif [[ -f "$DST/scripts/../tvbox-installer-menu.service" ]]; then
  cp -a "$DST/tvbox-installer-menu.service" /etc/systemd/system/tvbox-installer-menu.service
fi
if [[ -f "$DST/tvbox-installer-menu.service" ]]; then
  cp -a "$DST/tvbox-installer-menu.service" /etc/systemd/system/tvbox-installer-menu.service
fi
# service file lives next to scripts in deploy tarball
if [[ -f "$SRC/tvbox-installer-menu.service" ]]; then
  cp -a "$SRC/tvbox-installer-menu.service" "$DST/"
  cp -a "$SRC/tvbox-installer-menu.service" /etc/systemd/system/tvbox-installer-menu.service
fi

systemctl disable --now getty@tty1.service 2>/dev/null || true
systemctl mask getty@tty1.service 2>/dev/null || true
systemctl daemon-reload
systemctl enable tvbox-installer-menu.service

# fallback: autologin if someone unmasks getty
mkdir -p /etc/systemd/system/getty@.service.d
cat >/etc/systemd/system/getty@.service.d/autologin.conf <<'EOF'
[Service]
ExecStart=
ExecStart=-/sbin/agetty --autologin boxer --noclear %I linux
EOF

# start menu on tty1 login (fallback if getty is used)
cat >/home/boxer/.bash_profile <<'EOF'
# TVBox installer appliance
if [ "$(tty 2>/dev/null)" = "/dev/tty1" ]; then
  echo "Starting TVBox installer menu..."
  sudo /usr/local/sbin/tvbox-menu || sudo /opt/tvbox-installer/scripts/tvbox-installer-menu.sh || true
fi
EOF
chown boxer:boxer /home/boxer/.bash_profile

# helper alias
grep -q 'alias tvbox-menu=' /home/boxer/.bashrc 2>/dev/null || \
  echo 'alias tvbox-menu="sudo /usr/local/sbin/tvbox-menu"' >>/home/boxer/.bashrc

systemctl daemon-reload

# TV/monitor often rejects 4K console timing from Wyse DP
if [[ -x "$DST/scripts/fix_display_1080p.sh" ]]; then
  bash "$DST/scripts/fix_display_1080p.sh" || true
fi

echo "SETUP_OK DST=$DST"
ls -la "$DST/scripts"
