#!/usr/bin/env bash
# Capture golden Wyse (overlayroot) -> katalog OUT (root z /run/rootfsbase).
# Uzycie: sudo bash capture_golden_local.sh /path/out
set -euo pipefail

OUT="${1:?outdir}"
STAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DEST="$OUT/golden-$STAMP"
mkdir -p "$DEST"

EFI_PART="/dev/sda1"
BOOT_PART="/dev/sda2"
ROOT_SRC="/run/rootfsbase"
DATA_LV="/dev/mapper/ubuntu--vg-tvbox--data"

[[ -d "$ROOT_SRC" ]] || { echo "Brak $ROOT_SRC — overlayroot?"; exit 1; }
[[ -b "$EFI_PART" ]] || { echo "Brak $EFI_PART"; exit 1; }
[[ -b "$BOOT_PART" ]] || { echo "Brak $BOOT_PART"; exit 1; }

WORKDIR="/tmp/tvbox-capture-$$"
mkdir -p "$WORKDIR"/{efi,boot,data}
# Juz zamontowane na golden — bind
mount --bind /boot/efi "$WORKDIR/efi"
mount --bind /boot "$WORKDIR/boot"
mount --bind /home/boxer/tvbox/data "$WORKDIR/data"

echo "[capture] efi..."
tar -C "$WORKDIR/efi" -czf "$DEST/efi.tar.gz" .
echo "[capture] boot..."
tar -C "$WORKDIR/boot" -czf "$DEST/boot.tar.gz" .
echo "[capture] root (z $ROOT_SRC) — to potrwa..."
tar -C "$ROOT_SRC" \
  --exclude=./proc --exclude=./sys --exclude=./dev --exclude=./run \
  --exclude=./tmp --exclude=./mnt --exclude=./media --exclude=./lost+found \
  --exclude=./home/boxer/tvbox/data \
  -czf "$DEST/root.tar.gz" .
echo "[capture] data..."
tar -C "$WORKDIR/data" \
  --exclude=./lost+found \
  --exclude=./capture-* \
  --exclude=./.capture-* \
  -czf "$DEST/data.tar.gz" .

umount "$WORKDIR/data" "$WORKDIR/boot" "$WORKDIR/efi" || true
rm -rf "$WORKDIR"

{
  echo "stamp=$STAMP"
  echo "host=$(hostname)"
  date -u
  lsblk -b /dev/sda
  sha256sum "$DEST"/*.tar.gz
  du -h "$DEST"/*.tar.gz
} | tee "$DEST/manifest.txt"

ln -sfn "golden-$STAMP" "$OUT/current"
echo "CAPTURE_OK $DEST"
ls -lh "$DEST"
