#!/usr/bin/env bash
# Repair cloned Wyse that drops to dracut emergency (rdsosreport.txt).
# Cause: golden initrd bakes root=/dev/mapper/ubuntu--vg-ubuntu--lv (USB installer VG).
# Target eMMC uses tvbox-vg. Run from installer USB as root.
set -euo pipefail
[[ "$(id -u)" -eq 0 ]] || { echo "sudo $0"; exit 1; }

VG="${1:-tvbox-vg}"
ROOT_LV="/dev/${VG}/ubuntu-lv"
DISK="${2:-/dev/mmcblk0}"
if [[ "$DISK" == *mmcblk* || "$DISK" == *nvme* ]]; then
  BOOT_PART="${DISK}p2"
  EFI_PART="${DISK}p1"
else
  BOOT_PART="${DISK}2"
  EFI_PART="${DISK}1"
fi

echo "[fix] tearing down leftover mounts"
for m in \
  /mnt/mmc-root/run /mnt/mmc-root/sys /mnt/mmc-root/proc /mnt/mmc-root/dev \
  /mnt/mmc-root/boot/efi /mnt/mmc-root/boot \
  /mnt/wyse/run /mnt/wyse/sys /mnt/wyse/proc /mnt/wyse/dev \
  /mnt/wyse/boot/efi /mnt/wyse/boot \
  /mnt/mmc-root /mnt/mmc-boot /mnt/mmc-boot2 /mnt/wyse /mnt
do
  umount "$m" 2>/dev/null || true
done
umount -l /mnt/mmc-root 2>/dev/null || true
umount -l /mnt/wyse 2>/dev/null || true
umount -l /mnt 2>/dev/null || true

vgchange -ay "$VG" >/dev/null
[[ -b "$ROOT_LV" ]] || { echo "Brak $ROOT_LV"; exit 1; }

ROOT=/mnt/wyse
mkdir -p "$ROOT"
mount "$ROOT_LV" "$ROOT"
mkdir -p "$ROOT/boot"
mount "$BOOT_PART" "$ROOT/boot"
mkdir -p "$ROOT/boot/efi"
mount "$EFI_PART" "$ROOT/boot/efi"
[[ -d "$ROOT/boot/grub" ]] || { echo "Brak /boot/grub — zly mount boot"; exit 1; }
ls "$ROOT/boot"/vmlinuz-* >/dev/null

mkdir -p "$ROOT/etc/dracut.conf.d"
cat >"$ROOT/etc/dracut.conf.d/90-tvbox.conf" <<'EOF'
hostonly="no"
hostonly_cmdline="no"
EOF

if [[ -f "$ROOT/etc/default/grub" ]]; then
  sed -i '/^GRUB_CMDLINE_LINUX=/d' "$ROOT/etc/default/grub"
  echo "GRUB_CMDLINE_LINUX=\"video=DP-1:1920x1080@60 rd.lvm.lv=${VG}/ubuntu-lv rd.lvm.lv=${VG}/tvbox-data\"" >>"$ROOT/etc/default/grub"
fi

mount --bind /dev "$ROOT/dev"
mount --bind /proc "$ROOT/proc"
mount --bind /sys "$ROOT/sys"
mount --bind /run "$ROOT/run"
mkdir -p "$ROOT/tmp"

echo "[fix] dracut --regenerate-all + update-grub"
chroot "$ROOT" /bin/bash -c '
set -e
test -d /boot/grub
if command -v dracut >/dev/null; then
  dracut -f --regenerate-all
else
  update-initramfs -u -k all
fi
update-grub
'

echo "[fix] grub root lines:"
grep -E '^\s*linux\s' "$ROOT/boot/grub/grub.cfg" | head -3
umount "$ROOT/run" "$ROOT/sys" "$ROOT/proc" "$ROOT/dev" || true
echo FIX_OK
