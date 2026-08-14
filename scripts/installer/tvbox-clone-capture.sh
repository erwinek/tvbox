#!/usr/bin/env bash
# Capture golden Wyse (32GB): rsync/tar FS na partycje TVBOXIMG/VENTOY.
# Uruchom z Ubuntu live (USB), jako root.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "${SCRIPT_DIR}/common.sh"

require_root
export DEBIAN_FRONTEND=noninteractive

log "Instalacja narzedzi (jesli brak)..."
apt-get update -qq || true
apt-get install -y --no-install-recommends \
  rsync tar gzip lvm2 gdisk parted util-linux e2fsprogs dosfstools ca-certificates \
  >/dev/null

BOOT_DISK="$(boot_disk || true)"
log "Boot disk (USB/ISO): ${BOOT_DISK:-unknown}"
INTERNAL="$(pick_internal_disk "${BOOT_DISK:-}")" || die "Brak dysku wewnetrznego"
SIZE="$(disk_size_bytes "$INTERNAL")"
log "Internal disk: $INTERNAL ($SIZE bytes)"
list_disks
echo ""
lsblk "$INTERNAL"

confirm "CAPTURE z $INTERNAL na USB (nadpisze images/ na pendrive)"

MNT="$(mount_tvboximg /mnt/TVBOXIMG)"
IMG="$(images_dir "$MNT")"
STAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUT="$IMG/golden-$STAMP"
mkdir -p "$OUT"
ln -sfn "$(basename "$OUT")" "$IMG/current"

log "Aktywacja LVM..."
vgchange -ay || true
sleep 1

ROOT_LV="/dev/mapper/ubuntu--vg-ubuntu--lv"
DATA_LV="/dev/mapper/ubuntu--vg-tvbox--data"
# Partycje: ${INTERNAL}1 EFI, ${INTERNAL}2 boot — obsluga nvme p-suffix
if [[ "$INTERNAL" == *nvme* ]]; then
  EFI_PART="${INTERNAL}p1"
  BOOT_PART="${INTERNAL}p2"
else
  EFI_PART="${INTERNAL}1"
  BOOT_PART="${INTERNAL}2"
fi

[[ -b "$EFI_PART" ]] || die "Brak EFI $EFI_PART"
[[ -b "$BOOT_PART" ]] || die "Brak boot $BOOT_PART"
[[ -b "$ROOT_LV" ]] || die "Brak LV root $ROOT_LV — sprawdz vgchange -ay"
[[ -b "$DATA_LV" ]] || die "Brak LV data $DATA_LV"

WORKDIR="/mnt/tvbox-capture"
rm -rf "$WORKDIR"
mkdir -p "$WORKDIR"/{efi,boot,root,data}

mount -o ro "$EFI_PART" "$WORKDIR/efi"
mount -o ro "$BOOT_PART" "$WORKDIR/boot"
mount -o ro "$ROOT_LV" "$WORKDIR/root"
mount -o ro "$DATA_LV" "$WORKDIR/data"

log "Pakowanie efi..."
tar -C "$WORKDIR/efi" -czf "$OUT/efi.tar.gz" .

log "Pakowanie boot..."
tar -C "$WORKDIR/boot" -czf "$OUT/boot.tar.gz" .

log "Pakowanie root (to potrwa)..."
tar -C "$WORKDIR/root" \
  --exclude=./proc --exclude=./sys --exclude=./dev --exclude=./run \
  --exclude=./tmp --exclude=./mnt --exclude=./media --exclude=./lost+found \
  -czf "$OUT/root.tar.gz" .

log "Pakowanie data..."
tar -C "$WORKDIR/data" \
  --exclude=./lost+found \
  -czf "$OUT/data.tar.gz" .

umount "$WORKDIR/data" "$WORKDIR/root" "$WORKDIR/boot" "$WORKDIR/efi"
rmdir "$WORKDIR"/{efi,boot,root,data} 2>/dev/null || true
rmdir "$WORKDIR" 2>/dev/null || true

{
  echo "stamp=$STAMP"
  echo "source_disk=$INTERNAL"
  echo "source_size=$SIZE"
  echo "host=$(cat /etc/hostname 2>/dev/null || echo unknown)"
  date -u
  lsblk -b "$INTERNAL"
  vgs; lvs
  sha256sum "$OUT"/*.tar.gz
} | tee "$OUT/manifest.txt"

cp -a "$SCRIPT_DIR"/*.sh "$MNT/scripts/" 2>/dev/null || true
sync

log "CAPTURE OK -> $OUT"
log "Nastepnie: boot USB na target 16GB i: bash /mnt/TVBOXIMG/scripts/tvbox-clone-restore.sh"
