#!/usr/bin/env bash
# Partycjonowanie dysku docelowego pod Wyse 16GB (GPT + LVM).
# Argument: /dev/mmcblk0 | /dev/sdX
set -euo pipefail

DISK="${1:?disk}"
[[ -b "$DISK" ]] || { echo "Not a block device: $DISK" >&2; exit 1; }

if [[ "$DISK" == *nvme* || "$DISK" == *mmcblk* ]]; then
  P1="${DISK}p1"; P2="${DISK}p2"; P3="${DISK}p3"
else
  P1="${DISK}1"; P2="${DISK}2"; P3="${DISK}3"
fi

echo "[partition] Wipe LVM on TARGET $DISK only (installer VG stays)"
while read -r pv vg; do
  pv="${pv#"${pv%%[![:space:]]*}"}"
  vg="${vg#"${vg%%[![:space:]]*}"}"
  [[ -n "$pv" ]] || continue
  pk="$(lsblk -no PKNAME "$pv" 2>/dev/null | awk 'NF{print; exit}')"
  [[ "/dev/${pk}" == "$DISK" ]] || continue
  if [[ -n "$vg" ]]; then
    echo "[partition] vgremove $vg (on $pv)"
    vgchange -an "$vg" || true
    vgremove -y "$vg" || true
  fi
  pvremove -ff -y "$pv" || true
done < <(pvs --noheadings -o pv_name,vg_name 2>/dev/null || true)

echo "[partition] Wipe + GPT on $DISK"
wipefs -a "$DISK" || true
sgdisk --zap-all "$DISK"
partprobe "$DISK" || true
sleep 1

# EFI 256M, boot 768M, rest LVM
sgdisk -n 1:0:+256M -t 1:ef00 -c 1:"EFI System" "$DISK"
sgdisk -n 2:0:+768M -t 2:8300 -c 2:"boot" "$DISK"
sgdisk -n 3:0:0 -t 3:8e00 -c 3:"lvm" "$DISK"
partprobe "$DISK"
sleep 2

mkfs.vfat -F32 -n EFI "$P1"
mkfs.ext4 -F -L boot "$P2"

# USB installer already has ubuntu-vg — nie wolno zduplikowac nazwy
TARGET_VG="ubuntu-vg"
if vgs "$TARGET_VG" >/dev/null 2>&1; then
  TARGET_VG="tvbox-vg"
  echo "[partition] ubuntu-vg zajete przez installer — target VG=$TARGET_VG"
fi

pvcreate -ff -y "$P3"
vgcreate "$TARGET_VG" "$P3"
lvcreate -y -L 10G -n ubuntu-lv "$TARGET_VG"
lvcreate -y -L 3G -n tvbox-data "$TARGET_VG"
mkfs.ext4 -F -L root "/dev/${TARGET_VG}/ubuntu-lv"
mkfs.ext4 -F -L tvbox-data "/dev/${TARGET_VG}/tvbox-data"

cat >/run/tvbox-target.env <<EOF
TARGET_VG=${TARGET_VG}
ROOT_LV=/dev/${TARGET_VG}/ubuntu-lv
DATA_LV=/dev/${TARGET_VG}/tvbox-data
TARGET_DISK=${DISK}
EOF

echo "[partition] Done:"
cat /run/tvbox-target.env
lsblk "$DISK"
lvs
