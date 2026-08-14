#!/usr/bin/env bash
set -euo pipefail
USB=/dev/sdb
[[ "$(id -u)" -eq 0 ]] || exec sudo bash "$0" "$@"

umount ${USB}? ${USB}?? 2>/dev/null || true

echo "=== Fix GPT to end of disk ==="
sgdisk -e "$USB"
partprobe "$USB" || true
sleep 1

# Drop tiny leftover partition 3 if present
if [[ -b ${USB}3 ]]; then
  SIZE=$(lsblk -bdn -o SIZE "${USB}3")
  if (( SIZE < 10*1024*1024 )); then
    echo "Deleting tiny ${USB}3 ($SIZE bytes)"
    sgdisk -d 3 "$USB" || true
    partprobe "$USB" || true
    sleep 1
  fi
fi

sgdisk -e "$USB" || true
partprobe "$USB" || true

echo "=== parted free ==="
parted -s "$USB" unit MiB print free || true
lsblk -o NAME,SIZE,LABEL,FSTYPE "$USB"

FIRST_FREE=$(parted -sm "$USB" unit MiB print free | awk -F: '
  $1=="free" || $4=="free" {
    # parted -sm free lines look like: 1:2784MiB:59120MiB:56336MiB:free;
    gsub(/MiB/,"",$2); print int($2); exit
  }
  /:free;$/ {
    gsub(/MiB/,"",$2); print int($2); exit
  }
')
# Fallback parse
if [[ -z "${FIRST_FREE:-}" ]]; then
  FIRST_FREE=$(parted -sm "$USB" unit MiB print free | grep ':free;' | head -1 | cut -d: -f2 | tr -d 'MiB')
fi
echo "FIRST_FREE=${FIRST_FREE}"

[[ -n "${FIRST_FREE:-}" ]] || { echo "No free space found"; exit 1; }

# Next partition number
NEXT=4
[[ -b ${USB}3 ]] || NEXT=3

parted -s "$USB" mkpart primary ext4 "${FIRST_FREE}MiB" "100%"
partprobe "$USB" || true
sleep 2

NEW=$(lsblk -ln -o NAME,TYPE "$USB" | awk '$2=="part"{print "/dev/"$1}' | tail -1)
echo "NEW=$NEW"
mkfs.ext4 -F -L TVBOXIMG "$NEW"
mkdir -p /mnt/TVBOXIMG
mount "$NEW" /mnt/TVBOXIMG
rm -rf /mnt/TVBOXIMG/tvbox-installer
mkdir -p /mnt/TVBOXIMG/tvbox-installer/scripts /mnt/TVBOXIMG/images

TGZ=/home/boxer/tvbox/data/tvbox-installer-usb.tgz
if [[ -f "$TGZ" ]]; then
  tar -xzf "$TGZ" -C /mnt/TVBOXIMG/tvbox-installer/
  if [[ -f /mnt/TVBOXIMG/tvbox-installer/common.sh ]]; then
    cp -a /mnt/TVBOXIMG/tvbox-installer/*.sh /mnt/TVBOXIMG/tvbox-installer/scripts/ 2>/dev/null || true
  fi
fi
find /mnt/TVBOXIMG -name '*.sh' -exec chmod +x {} +
find /mnt/TVBOXIMG -name '*.sh' -exec sed -i 's/\r$//' {} +
df -h /mnt/TVBOXIMG
ls -la /mnt/TVBOXIMG/tvbox-installer/scripts/
sync
umount /mnt/TVBOXIMG

rm -f /home/boxer/tvbox/data/ubuntu-26.04-live-server-amd64.iso
lsblk -o NAME,SIZE,TYPE,LABEL,FSTYPE "$USB"
echo FIX_DONE
