#!/usr/bin/env bash
# Restore golden capture na target Wyse 16GB SSD.
# Uruchom z Ubuntu live (USB), jako root.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "${SCRIPT_DIR}/common.sh"

require_root
export DEBIAN_FRONTEND=noninteractive

log "Instalacja narzedzi..."
apt-get update -qq || true
apt-get install -y --no-install-recommends \
  rsync tar gzip lvm2 gdisk parted util-linux e2fsprogs dosfstools \
  grub-efi-amd64-bin efibootmgr ca-certificates \
  >/dev/null

BOOT_DISK="$(boot_disk || true)"
INTERNAL="$(pick_internal_disk "${BOOT_DISK:-}")" || die "Brak dysku wewnetrznego"
SIZE="$(disk_size_bytes "$INTERNAL")"
log "Target disk: $INTERNAL ($SIZE bytes)"
list_disks
lsblk "$INTERNAL"

(( SIZE >= MIN_TARGET_BYTES )) || die "Dysk za maly ($SIZE < $MIN_TARGET_BYTES)"
# Ostrzezenie gdy ktos wlozy 32GB golden jako target przypadkiem — OK, tez zadziała
confirm "UWAGA: WYMAZE CALY $INTERNAL i wgra obraz TVBox (layout 16GB)"

MNT="$(mount_tvboximg /mnt/TVBOXIMG)"
IMG="$(images_dir "$MNT")"
CUR="$IMG/current"
[[ -L "$CUR" || -d "$CUR" ]] || die "Brak $IMG/current — najpierw capture"
SRC="$(readlink -f "$CUR")"
[[ -f "$SRC/root.tar.gz" ]] || die "Brak $SRC/root.tar.gz"

bash "${SCRIPT_DIR}/tvbox-clone-partition.sh" "$INTERNAL"

if [[ "$INTERNAL" == *nvme* ]]; then
  EFI_PART="${INTERNAL}p1"; BOOT_PART="${INTERNAL}p2"
else
  EFI_PART="${INTERNAL}1"; BOOT_PART="${INTERNAL}2"
fi
ROOT_LV="/dev/ubuntu-vg/ubuntu-lv"
DATA_LV="/dev/ubuntu-vg/tvbox-data"

TARGET="/mnt/tvbox-target"
rm -rf "$TARGET"
mkdir -p "$TARGET"/{efi,boot,root,data}

mount "$ROOT_LV" "$TARGET/root"
mount "$BOOT_PART" "$TARGET/boot"
mount "$EFI_PART" "$TARGET/efi"
mount "$DATA_LV" "$TARGET/data"

# Bind for grub later
mkdir -p "$TARGET/root/boot" "$TARGET/root/boot/efi"
mount --bind "$TARGET/boot" "$TARGET/root/boot"
mount --bind "$TARGET/efi" "$TARGET/root/boot/efi"

log "Extract efi..."
tar -C "$TARGET/efi" -xzf "$SRC/efi.tar.gz"
log "Extract boot..."
tar -C "$TARGET/boot" -xzf "$SRC/boot.tar.gz"
log "Extract root..."
tar -C "$TARGET/root" -xzf "$SRC/root.tar.gz"
log "Extract data..."
tar -C "$TARGET/data" -xzf "$SRC/data.tar.gz"

# fstab z nowymi UUID
EFI_UUID="$(blkid -s UUID -o value "$EFI_PART")"
BOOT_UUID="$(blkid -s UUID -o value "$BOOT_PART")"
ROOT_UUID="$(blkid -s UUID -o value "$ROOT_LV")"
DATA_UUID="$(blkid -s UUID -o value "$DATA_LV")"

cat >"$TARGET/root/etc/fstab" <<EOF
# TVBox clone restore $(date -u -Iseconds)
UUID=${ROOT_UUID} / ext4 defaults,noatime,nodiratime,commit=60,errors=remount-ro 0 1
UUID=${BOOT_UUID} /boot ext4 defaults,noatime,nodiratime,commit=60,errors=remount-ro 0 2
UUID=${EFI_UUID} /boot/efi vfat umask=0077,noatime 0 1
UUID=${DATA_UUID} /home/boxer/tvbox/data ext4 defaults,noatime,nodiratime,commit=60,errors=remount-ro 0 2
EOF

# overlayroot zachowaj / ustaw
mkdir -p "$TARGET/root/etc"
if [[ ! -f "$TARGET/root/etc/overlayroot.conf" ]]; then
  cat >"$TARGET/root/etc/overlayroot.conf" <<'EOF'
overlayroot_cfgdisk="disabled"
overlayroot="tmpfs"
EOF
fi

# First-boot hook
install -m 0755 "${SCRIPT_DIR}/tvbox-clone-firstboot.sh" \
  "$TARGET/root/usr/local/sbin/tvbox-clone-firstboot.sh"
mkdir -p "$TARGET/root/etc/systemd/system"
cat >"$TARGET/root/etc/systemd/system/tvbox-clone-firstboot.service" <<'EOF'
[Unit]
Description=TVBox clone first-boot identity
After=local-fs.target
ConditionPathExists=/home/boxer/tvbox/data/.tvbox-needs-firstboot

[Service]
Type=oneshot
ExecStart=/usr/local/sbin/tvbox-clone-firstboot.sh
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
EOF

mkdir -p "$TARGET/data"
touch "$TARGET/data/.tvbox-needs-firstboot"
# Czysty ranking na nowej maszynie
rm -f "$TARGET/data/leaderboard.db" "$TARGET/data/leaderboard.db-*" 2>/dev/null || true
chown -R 1000:1000 "$TARGET/data" 2>/dev/null || true

# Enable unit via symlink (bez systemctl w chroot jeszcze)
mkdir -p "$TARGET/root/etc/systemd/system/multi-user.target.wants"
ln -sfn /etc/systemd/system/tvbox-clone-firstboot.service \
  "$TARGET/root/etc/systemd/system/multi-user.target.wants/tvbox-clone-firstboot.service"

log "GRUB EFI..."
mount --bind /dev "$TARGET/root/dev"
mount --bind /proc "$TARGET/root/proc"
mount --bind /sys "$TARGET/root/sys"
mount --bind /run "$TARGET/root/run"

chroot "$TARGET/root" /bin/bash -c "
set -e
export DEBIAN_FRONTEND=noninteractive
grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=ubuntu --recheck || true
update-grub || true
# machine-id wyczysc — firstboot wygeneruje
truncate -s 0 /etc/machine-id || true
rm -f /var/lib/dbus/machine-id || true
"

umount "$TARGET/root/run" "$TARGET/root/sys" "$TARGET/root/proc" "$TARGET/root/dev" || true
umount "$TARGET/root/boot/efi" "$TARGET/root/boot" || true
umount "$TARGET/data" "$TARGET/efi" "$TARGET/boot" "$TARGET/root" || true

sync
log "RESTORE OK. Wyjmij USB i zrebootuj target."
