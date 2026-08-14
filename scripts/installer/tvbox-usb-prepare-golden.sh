#!/usr/bin/env bash
# Przygotuj pendrive NA DZIALAJACYM golden Wyse (przez SSH).
# Wymaga: USB Ventoy wlozony w golden (nie w PC).
# Capture z /run/rootfsbase + /boot + EFI + tvbox-data (bez live USB).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "${SCRIPT_DIR}/common.sh"

require_root

pick_usb_disk() {
  local name size type tran model
  while read -r name size type tran model; do
    [[ "$type" == "disk" ]] || continue
    [[ "${tran,,}" == "usb" ]] || continue
    echo "/dev/$name"
    return 0
  done < <(list_disks)
  return 1
}

part_for() {
  local disk="$1" n="$2"
  if [[ "$disk" == *nvme* ]]; then echo "${disk}p${n}"; else echo "${disk}${n}"; fi
}

log "Szukam USB (Ventoy)..."
list_disks
USB="$(pick_usb_disk)" || die "Brak dysku USB — wloz pendrive Ventoy do golden i sprobuj ponownie"
log "USB: $USB ($(disk_size_bytes "$USB") bytes)"
lsblk "$USB"

# Nie capture'uj na wewnetrzny dysk przez pomylke
INTERNAL="$(pick_internal_disk "")" || true
[[ -n "${INTERNAL:-}" && "$USB" == "$INTERNAL" ]] && die "USB == internal — abort"

MNT="/mnt/TVBOXIMG"
mkdir -p "$MNT"
# Odmontuj ewentualne auto-mounty
for p in $(lsblk -ln -o NAME,TYPE "$USB" | awk '$2=="part"{print "/dev/"$1}'); do
  umount "$p" 2>/dev/null || true
done

# Prefer label TVBOXIMG (data), potem Ventoy
DEV=""
for lab in TVBOXIMG Ventoy VENTOY; do
  DEV="$(blkid -L "$lab" 2>/dev/null || true)"
  [[ -n "$DEV" ]] && break
done
if [[ -z "$DEV" ]]; then
  # ostatnia partycja na USB zwykle data
  DEV="$(lsblk -ln -o NAME,TYPE "$USB" | awk '$2=="part"{print "/dev/"$1}' | tail -1)"
fi
[[ -b "$DEV" ]] || die "Brak partycji danych na $USB"
# Nie montuj iso9660 jako data
FSTYPE="$(blkid -s TYPE -o value "$DEV" 2>/dev/null || true)"
[[ "$FSTYPE" == "iso9660" ]] && die "Zla partycja ($DEV iso9660) — brak TVBOXIMG?"
mount "$DEV" "$MNT" || die "mount $DEV failed"
log "Mounted $DEV ($FSTYPE) -> $MNT"

# Katalogi
mkdir -p "$MNT/tvbox-installer/scripts" "$MNT/images"
IMG="$MNT/images"
STAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUT="$IMG/golden-$STAMP"
mkdir -p "$OUT"
ln -sfn "$(basename "$OUT")" "$IMG/current"

# Skrypty + auto-restore
cp -a "$SCRIPT_DIR"/*.sh "$MNT/tvbox-installer/scripts/"
cp -a "$SCRIPT_DIR/README.md" "$MNT/tvbox-installer/" 2>/dev/null || true
if [[ -f "$SCRIPT_DIR/ventoy/ventoy.json" ]]; then
  mkdir -p "$MNT/ventoy"
  cp -a "$SCRIPT_DIR/ventoy/ventoy.json" "$MNT/ventoy/ventoy.json"
fi
if [[ -f "$SCRIPT_DIR/ventoy/inject.tar.gz" ]]; then
  cp -a "$SCRIPT_DIR/ventoy/inject.tar.gz" "$MNT/tvbox-installer/inject.tar.gz"
fi

# Capture z dzialajacego systemu (lower overlay = /run/rootfsbase)
ROOT_SRC="/run/rootfsbase"
[[ -d "$ROOT_SRC/usr" ]] || die "Brak $ROOT_SRC — overlayroot nieaktywny? Capture z live."
BOOT_SRC="/boot"
EFI_SRC="/boot/efi"
DATA_SRC="/home/boxer/tvbox/data"
[[ -d "$BOOT_SRC" ]] || die "Brak $BOOT_SRC"
[[ -d "$EFI_SRC" ]] || die "Brak $EFI_SRC"
[[ -d "$DATA_SRC" ]] || die "Brak $DATA_SRC"

log "Pakowanie EFI..."
tar -C "$EFI_SRC" -czf "$OUT/efi.tar.gz" .

log "Pakowanie boot..."
tar -C "$BOOT_SRC" \
  --exclude=./efi \
  -czf "$OUT/boot.tar.gz" .

log "Pakowanie root (lower) — to potrwa..."
tar -C "$ROOT_SRC" \
  --exclude=./proc --exclude=./sys --exclude=./dev --exclude=./run \
  --exclude=./tmp --exclude=./mnt --exclude=./media --exclude=./lost+found \
  --exclude=./boot \
  -czf "$OUT/root.tar.gz" .

log "Pakowanie data..."
tar -C "$DATA_SRC" \
  --exclude=./lost+found \
  --exclude=./videos \
  -czf "$OUT/data.tar.gz" .

{
  echo "stamp=$STAMP"
  echo "mode=golden-ssh-live"
  echo "usb=$USB"
  hostname
  date -u
  df -h "$ROOT_SRC" "$BOOT_SRC" "$EFI_SRC" "$DATA_SRC"
  sha256sum "$OUT"/*.tar.gz
} | tee "$OUT/manifest.txt"

# Flaga: po boot live na targetcie auto-restore
touch "$MNT/tvbox-installer/DO_RESTORE"
rm -f "$MNT/tvbox-installer/RESTORE_DONE" 2>/dev/null || true

sync
umount "$MNT" || true

log "OK — pendrive gotowy."
log "1) Wyjmij USB z golden"
log "2) Wloz do target 16GB, boot z USB (F12) -> Ubuntu"
log "3) Auto-restore odpali sie sam (Ventoy injection) albo:"
log "   sudo bash /media/*/tvbox-installer/scripts/tvbox-auto-restore.sh"
