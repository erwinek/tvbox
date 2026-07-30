#!/usr/bin/env bash
# Wariant 2: root przez overlayroot (tmpfs) + osobny LV RW na tvbox data.
# Idempotentny. Po zakonczeniu: sudo reboot
set -euo pipefail

if [[ "$(id -u)" -ne 0 ]]; then
  echo "Uruchom: sudo bash $0"
  exit 1
fi

VG="ubuntu-vg"
LV="tvbox-data"
LV_PATH="/dev/${VG}/${LV}"
DATA_MNT="/home/boxer/tvbox/data"
DATA_SIZE="${TVBOX_DATA_SIZE:-8G}"

echo "=== TVBox readonly root + RW data volume ==="
echo "LV: $LV_PATH ($DATA_SIZE) -> $DATA_MNT"

echo ""
echo "--- [1/7] Stop kiosk ---"
systemctl stop tvbox 2>/dev/null || true
sleep 1

echo ""
echo "--- [2/7] Create LVM volume (if needed) ---"
if [[ ! -e "$LV_PATH" ]]; then
  FREE="$(vgs --noheadings -o vg_free --units g "$VG" | tr -d ' ')"
  echo "VG free: $FREE"
  lvcreate -y -L "$DATA_SIZE" -n "$LV" "$VG"
  mkfs.ext4 -F -L tvbox-data -E lazy_itable_init=0,lazy_journal_init=0 "$LV_PATH"
  tune2fs -c 2 -i 1d "$LV_PATH" || true
  echo "Created and formatted $LV_PATH"
else
  echo "LV already exists: $LV_PATH"
  # Upewnij sie ze ma filesystem
  if ! blkid -o value -s TYPE "$LV_PATH" | grep -q ext4; then
    mkfs.ext4 -F -L tvbox-data "$LV_PATH"
  fi
fi

UUID="$(blkid -s UUID -o value "$LV_PATH")"
echo "UUID=$UUID"

echo ""
echo "--- [3/7] Migrate data ---"
# Jesli data juz jest zamontowanym LV — nic nie ruszaj
if findmnt -n "$DATA_MNT" 2>/dev/null | grep -q "$LV\|tvbox-data\|$UUID"; then
  echo "Data already on dedicated volume"
else
  mkdir -p /mnt/tvbox-data-mig
  mount "$LV_PATH" /mnt/tvbox-data-mig

  if [[ -d "$DATA_MNT" ]]; then
    echo "Copying existing $DATA_MNT -> LV..."
    rsync -aHAX --info=stats2 "$DATA_MNT"/ /mnt/tvbox-data-mig/ || \
      rsync -a "$DATA_MNT"/ /mnt/tvbox-data-mig/
    # Zachowaj stary katalog na root (po overlay bedzie "znikac" i tak)
    if [[ ! -d "${DATA_MNT}.root-bak" ]]; then
      mv "$DATA_MNT" "${DATA_MNT}.root-bak"
    else
      rm -rf "$DATA_MNT"
    fi
  fi

  mkdir -p "$DATA_MNT"
  chown boxer:boxer "$DATA_MNT"
  umount /mnt/tvbox-data-mig
  rmdir /mnt/tvbox-data-mig 2>/dev/null || true
fi

echo ""
echo "--- [4/7] fstab entry ---"
# Usun stare wpisy tvbox-data / ten UUID
sed -i '/tvbox-data/d' /etc/fstab
sed -i "\|$UUID|d" /etc/fstab
sed -i '\|/home/boxer/tvbox/data|d' /etc/fstab

echo "UUID=${UUID} ${DATA_MNT} ext4 defaults,noatime,nodiratime,commit=60,errors=remount-ro 0 2" >> /etc/fstab
mkdir -p "$DATA_MNT"
chown boxer:boxer "$DATA_MNT"
mount "$DATA_MNT"
chown -R boxer:boxer "$DATA_MNT"
findmnt "$DATA_MNT"
df -h "$DATA_MNT"
ls -la "$DATA_MNT" | head

echo ""
echo "--- [5/7] overlayroot (tmpfs upper) ---"
apt-get install -y --no-install-recommends overlayroot

# Konfiguracja: root RO + upper w RAM. Osobny mount data zostaje RW.
if [[ -f /etc/overlayroot.conf ]]; then
  cp -a /etc/overlayroot.conf /etc/overlayroot.conf.tvbox.bak 2>/dev/null || true
fi
cat >/etc/overlayroot.conf <<'EOF'
# TVBox kiosk: system w overlay (tmpfs). Trwale RW tylko /home/boxer/tvbox/data (osobny LV).
# Update OS: boot z overlayroot=disabled w GRUB, albo: sudo overlayroot-chroot
overlayroot_cfgdisk="disabled"
overlayroot="tmpfs"
EOF

# Kernel cmdline tez (niektore wersje czytaja stamtad)
if [[ -f /etc/default/grub ]]; then
  if ! grep -q 'overlayroot=' /etc/default/grub; then
    sed -i 's|^GRUB_CMDLINE_LINUX_DEFAULT="\(.*\)"|GRUB_CMDLINE_LINUX_DEFAULT="\1 overlayroot=tmpfs"|' /etc/default/grub
  fi
  update-grub 2>/dev/null || true
fi

echo ""
echo "--- [6/7] systemd: czekaj na mount data ---"
mkdir -p /etc/systemd/system/tvbox.service.d
cat >/etc/systemd/system/tvbox.service.d/data-mount.conf <<'EOF'
[Unit]
RequiresMountsFor=/home/boxer/tvbox/data
After=local-fs.target
EOF
systemctl daemon-reload

# Swap na pliku w root RO bywa problematyczny — zostawiamy, awaria swap nie blokuje bootu.
# Opcjonalnie wylacz:
# swapoff -a; sed -i '/swap.img/s/^/#/' /etc/fstab

echo ""
echo "--- [7/7] Start kiosk (jeszcze bez overlay — po reboot) ---"
systemctl start tvbox 2>/dev/null || true
sleep 2
systemctl is-active tvbox || true

echo ""
echo "=== GOTOWE — wymagany reboot ==="
echo "Po reboot:"
echo "  - findmnt /          → overlay / overlayroot"
echo "  - findmnt $DATA_MNT  → /dev/ubuntu-vg/tvbox-data (rw)"
echo "  - touch /test        → znika po restarcie"
echo "  - touch $DATA_MNT/x  → zostaje po restarcie"
echo ""
echo "Update pakietow (gdy trzeba):"
echo "  sudo reboot  # z GRUB: edit, overlayroot=disabled"
echo "  # albo: sudo overlayroot-chroot"
echo ""
echo "Uruchom: sudo reboot"
