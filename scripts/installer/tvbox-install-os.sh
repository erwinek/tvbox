#!/usr/bin/env bash
# Opcja 2: full OS z USB payload/os na dysk wewnetrzny.
set -euo pipefail

INSTALL_ROOT="${INSTALL_ROOT:-/opt/tvbox-installer}"
# shellcheck source=common.sh
source "${INSTALL_ROOT}/scripts/common.sh"
require_root
export TVBOX_ASSUME_YES=1
export DEBIAN_FRONTEND=noninteractive

LOG="/var/log/tvbox-install-os.log"
: >"$LOG"
log() { echo "[$(date -u +%H:%M:%S)] $*" | tee -a "$LOG"; }
trap 'ec=$?; echo; echo "===== INSTALL FAILED (exit ${ec}) =====" | tee -a "$LOG"; echo "Log: $LOG"; exit "$ec"' ERR

TARGET="/mnt/tvbox-target"

cleanup_target_mounts() {
  log "Odmontowuje leftover $TARGET ..."
  if findmnt "$TARGET" >/dev/null 2>&1; then
    umount -R "$TARGET" || true
  fi
  umount -R "${TARGET}/root" 2>/dev/null || true
  for m in \
    "${TARGET}/root/boot/efi" "${TARGET}/root/boot" \
    "${TARGET}/root/dev" "${TARGET}/root/proc" "${TARGET}/root/sys" "${TARGET}/root/run" \
    "${TARGET}/boot/efi" "${TARGET}/boot" "${TARGET}/efi" "${TARGET}/data" \
    "${TARGET}/root" "${TARGET}"
  do
    umount "$m" 2>/dev/null || true
  done
}

PAYLOAD_OS="${TVBOX_PAYLOAD_OS:-${INSTALL_ROOT}/payload/os}"

if [[ -L "${PAYLOAD_OS}/current" || -d "${PAYLOAD_OS}/current" ]]; then
  SRC="$(readlink -f "${PAYLOAD_OS}/current")"
elif [[ -f "${PAYLOAD_OS}/root.tar.gz" ]]; then
  SRC="$PAYLOAD_OS"
else
  die "Brak OS payload w $PAYLOAD_OS"
fi
[[ -f "$SRC/root.tar.gz" ]] || die "Brak $SRC/root.tar.gz"

BOOT="$(boot_disk || true)"
INTERNAL="$(pick_internal_disk "${BOOT:-}" || true)"
[[ -n "$INTERNAL" ]] || die "Brak dysku wewnetrznego"

log "OS restore -> $INTERNAL from $SRC"
SIZE="$(disk_size_bytes "$INTERNAL")"
(( SIZE >= MIN_TARGET_BYTES )) || die "Dysk za maly ($SIZE)"

cleanup_target_mounts

bash "${INSTALL_ROOT}/scripts/tvbox-clone-partition.sh" "$INTERNAL"
# shellcheck disable=SC1091
source /run/tvbox-target.env
[[ -n "${ROOT_LV:-}" && -n "${DATA_LV:-}" ]] || die "Brak /run/tvbox-target.env po partycjonowaniu"
LIVE_ROOT="$(findmnt -n -o SOURCE / || true)"
[[ "$ROOT_LV" != "$LIVE_ROOT" ]] || die "Odmowa: ROOT_LV to dysk instalera ($ROOT_LV)"
log "Target VG=${TARGET_VG} ROOT_LV=$ROOT_LV DATA_LV=$DATA_LV"

if [[ "$INTERNAL" == *nvme* || "$INTERNAL" == *mmcblk* ]]; then
  EFI_PART="${INTERNAL}p1"
  BOOT_PART="${INTERNAL}p2"
else
  EFI_PART="${INTERNAL}1"
  BOOT_PART="${INTERNAL}2"
fi

log "Mount nested: root -> boot -> efi, data"
mkdir -p "$TARGET"
mount "$ROOT_LV" "$TARGET"
mkdir -p "$TARGET/boot"
mount "$BOOT_PART" "$TARGET/boot"
mkdir -p "$TARGET/boot/efi"
mount "$EFI_PART" "$TARGET/boot/efi"
mkdir -p "$TARGET/home/boxer/tvbox/data"
mount "$DATA_LV" "$TARGET/home/boxer/tvbox/data"

log "Extract efi..."
tar -C "$TARGET/boot/efi" -xzf "$SRC/efi.tar.gz"
log "Extract boot..."
tar -C "$TARGET/boot" -xzf "$SRC/boot.tar.gz"
log "Extract root (ok. 2GB, 2-5 min)..."
tar -C "$TARGET" --exclude=./boot --exclude=./home/boxer/tvbox/data \
  --exclude=./proc --exclude=./sys --exclude=./dev --exclude=./run \
  --exclude=./tmp --exclude=./mnt --exclude=./media \
  -xzf "$SRC/root.tar.gz"
log "Extract data..."
tar -C "$TARGET/home/boxer/tvbox/data" -xzf "$SRC/data.tar.gz"
log "Extract done"

mkdir -p "$TARGET/proc" "$TARGET/sys" "$TARGET/dev" "$TARGET/run" "$TARGET/tmp" "$TARGET/mnt"

EFI_UUID="$(blkid -s UUID -o value "$EFI_PART")"
BOOT_UUID="$(blkid -s UUID -o value "$BOOT_PART")"
ROOT_UUID="$(blkid -s UUID -o value "$ROOT_LV")"
DATA_UUID="$(blkid -s UUID -o value "$DATA_LV")"
log "UUIDs root=$ROOT_UUID boot=$BOOT_UUID efi=$EFI_UUID data=$DATA_UUID"

cat >"$TARGET/etc/fstab" <<EOF
UUID=${ROOT_UUID} / ext4 defaults,noatime,nodiratime,commit=60,errors=remount-ro 0 1
UUID=${BOOT_UUID} /boot ext4 defaults,noatime,nodiratime,commit=60,errors=remount-ro 0 2
UUID=${EFI_UUID} /boot/efi vfat umask=0077,noatime 0 1
UUID=${DATA_UUID} /home/boxer/tvbox/data ext4 defaults,noatime,nodiratime,commit=60,errors=remount-ro 0 2
EOF

install -m 0755 "${INSTALL_ROOT}/scripts/tvbox-clone-firstboot.sh" \
  "$TARGET/usr/local/sbin/tvbox-clone-firstboot.sh"
mkdir -p "$TARGET/etc/systemd/system/multi-user.target.wants"
cat >"$TARGET/etc/systemd/system/tvbox-clone-firstboot.service" <<'EOF'
[Unit]
Description=TVBox clone first-boot identity
After=local-fs.target
ConditionPathExists=/home/boxer/tvbox/data/.tvbox-needs-firstboot
[Service]
Type=oneshot
ExecStart=/usr/local/sbin/tvbox-clone-firstboot.sh
[Install]
WantedBy=multi-user.target
EOF
ln -sfn /etc/systemd/system/tvbox-clone-firstboot.service \
  "$TARGET/etc/systemd/system/multi-user.target.wants/tvbox-clone-firstboot.service"
touch "$TARGET/home/boxer/tvbox/data/.tvbox-needs-firstboot"
rm -f "$TARGET/home/boxer/tvbox/data/leaderboard.db" 2>/dev/null || true
chown -R 1000:1000 "$TARGET/home/boxer/tvbox/data" 2>/dev/null || true

if [[ -d "${INSTALL_ROOT}/payload/app/bin" ]]; then
  log "Kopiuje payload/app -> data/app/current"
  mkdir -p "$TARGET/home/boxer/tvbox/data/app/current"
  rsync -a "${INSTALL_ROOT}/payload/app"/ "$TARGET/home/boxer/tvbox/data/app/current"/
fi

if [[ -f "$TARGET/etc/default/grub" ]]; then
  sed -i '/^GRUB_DISABLE_LINUX_UUID=/d' "$TARGET/etc/default/grub"
  echo 'GRUB_DISABLE_LINUX_UUID=false' >>"$TARGET/etc/default/grub"
fi

mount --bind /dev "$TARGET/dev"
mount --bind /proc "$TARGET/proc"
mount --bind /sys "$TARGET/sys"
mount --bind /run "$TARGET/run"

if [[ -x "${INSTALL_ROOT}/scripts/fix_display_1080p.sh" ]]; then
  mkdir -p "$TARGET/tmp"
  cp -a "${INSTALL_ROOT}/scripts/fix_display_1080p.sh" "$TARGET/tmp/fix_display_1080p.sh"
  log "GRUB 1080p..."
  chroot "$TARGET" /bin/bash /tmp/fix_display_1080p.sh || true
fi

log "grub-install + update-grub + rebuild initramfs (dracut hostonly z golden psuje klon)..."
mkdir -p "$TARGET/etc/dracut.conf.d"
cat >"$TARGET/etc/dracut.conf.d/90-tvbox.conf" <<'EOF'
hostonly="no"
hostonly_cmdline="no"
EOF
if [[ -f "$TARGET/etc/default/grub" ]]; then
  sed -i '/^GRUB_CMDLINE_LINUX=/d' "$TARGET/etc/default/grub"
  echo "GRUB_CMDLINE_LINUX=\"video=DP-1:1920x1080@60 rd.lvm.lv=${TARGET_VG}/ubuntu-lv rd.lvm.lv=${TARGET_VG}/tvbox-data\"" >>"$TARGET/etc/default/grub"
  sed -i 's/^GRUB_TIMEOUT=.*/GRUB_TIMEOUT=0/' "$TARGET/etc/default/grub"
  sed -i 's/^GRUB_TIMEOUT_STYLE=.*/GRUB_TIMEOUT_STYLE=hidden/' "$TARGET/etc/default/grub"
  grep -q '^GRUB_TIMEOUT=' "$TARGET/etc/default/grub" || echo 'GRUB_TIMEOUT=0' >>"$TARGET/etc/default/grub"
  grep -q '^GRUB_TIMEOUT_STYLE=' "$TARGET/etc/default/grub" || echo 'GRUB_TIMEOUT_STYLE=hidden' >>"$TARGET/etc/default/grub"
fi

# USB kamera czesto enumeruje sie po starcie kiosku
mkdir -p "$TARGET/usr/local/sbin" "$TARGET/etc/systemd/system/tvbox.service.d"
cat >"$TARGET/usr/local/sbin/tvbox-wait-camera.sh" <<'EOS'
#!/bin/bash
for _ in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
  [ -e /dev/video0 ] && exit 0
  sleep 1
done
exit 0
EOS
chmod +x "$TARGET/usr/local/sbin/tvbox-wait-camera.sh"
cat >"$TARGET/etc/systemd/system/tvbox.service.d/wait-camera.conf" <<'EOF'
[Service]
ExecStartPre=/usr/local/sbin/tvbox-wait-camera.sh
EOF
chroot "$TARGET" /bin/bash -c "
set -e
grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=ubuntu --recheck || true
if command -v dracut >/dev/null; then
  dracut -f --regenerate-all || true
else
  update-initramfs -u -k all || true
fi
update-grub || true
truncate -s 0 /etc/machine-id || true
rm -f /var/lib/dbus/machine-id || true
"

umount "$TARGET/run" "$TARGET/sys" "$TARGET/proc" "$TARGET/dev" || true
umount "$TARGET/boot/efi" "$TARGET/boot" || true
umount "$TARGET/home/boxer/tvbox/data" || true
umount "$TARGET" || true
sync

log "===== INSTALL OK ====="
log "Wyjmij USB i zbootuj z dysku Wyse. Log: $LOG"
whiptail --msgbox "OK: caly OS wgrany.\n\nWyjmij pendrive i zrestartuj.\nLog: $LOG" 12 60 || true
