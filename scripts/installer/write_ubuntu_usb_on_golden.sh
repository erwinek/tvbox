#!/usr/bin/env bash
# Run on golden: wipe USB Ventoy, write Ubuntu live-server ISO, add TVBOXIMG + scripts.
set -euo pipefail

ISO="${ISO:-/home/boxer/tvbox/data/ubuntu-26.04-live-server-amd64.iso}"
SCR_TGZ="${SCR_TGZ:-/home/boxer/tvbox/data/tvbox-installer-usb.tgz}"
USB="${USB:-/dev/sdb}"

if [[ "$(id -u)" -ne 0 ]]; then
  exec sudo bash "$0" "$@"
fi

[[ -f "$ISO" ]] || { echo "Brak ISO: $ISO"; exit 1; }
[[ -b "$USB" ]] || { echo "Brak USB: $USB"; exit 1; }
# safety: must be USB transport
TRAN="$(lsblk -dn -o TRAN "$USB" 2>/dev/null || true)"
[[ "$TRAN" == "usb" ]] || { echo "ABORT: $USB TRAN=$TRAN (oczekiwano usb)"; exit 1; }

echo "=== Unmount ${USB}* ==="
umount ${USB}? ${USB}?? 2>/dev/null || true
sleep 1

echo "=== dd $ISO -> $USB ==="
dd if="$ISO" of="$USB" bs=4M status=progress oflag=sync,direct
sync
sleep 2
partprobe "$USB" || true
sleep 2
echo "=== After ISO ==="
fdisk -l "$USB" || true
lsblk -o NAME,SIZE,TYPE,LABEL,FSTYPE "$USB"

export DEBIAN_FRONTEND=noninteractive
apt-get update -qq || true
apt-get install -y --no-install-recommends parted 2>/dev/null || true
apt-get install -y --no-install-recommends exfatprogs 2>/dev/null || \
  apt-get install -y --no-install-recommends exfat-utils 2>/dev/null || true

END_MB=$(parted -sm "$USB" unit MiB print | awk -F: '
  /^[0-9]+:/ { e=$3; gsub(/MiB/,"",e) }
  END { if (e!="") print int(e)+1 }
')
DISK_MB=$(parted -sm "$USB" unit MiB print | awk -F: 'NR==2 { gsub(/MiB/,"",$2); print int($2) }')
echo "Last end=${END_MB}MiB disk=${DISK_MB}MiB"

if [[ -n "${END_MB:-}" && -n "${DISK_MB:-}" && $((DISK_MB - END_MB)) -gt 2048 ]]; then
  echo "=== mkpart TVBOXIMG ${END_MB}MiB-100% ==="
  parted -s "$USB" mkpart primary "${END_MB}MiB" "100%" || \
    parted -s "$USB" mkpart TVBOXIMG ext4 "${END_MB}MiB" "100%" || true
  partprobe "$USB" || true
  sleep 2
  NEW=$(lsblk -ln -o NAME,TYPE "$USB" | awk '$2=="part"{print "/dev/"$1}' | tail -1)
  echo "New partition: $NEW"
  if mkfs.exfat -n TVBOXIMG "$NEW" 2>/dev/null; then
    echo "exFAT OK"
  else
    mkfs.ext4 -F -L TVBOXIMG "$NEW"
  fi
  mkdir -p /mnt/TVBOXIMG
  mount "$NEW" /mnt/TVBOXIMG
  rm -rf /mnt/TVBOXIMG/tvbox-installer
  mkdir -p /mnt/TVBOXIMG/tvbox-installer/scripts /mnt/TVBOXIMG/images
  if [[ -f "$SCR_TGZ" ]]; then
    tar -xzf "$SCR_TGZ" -C /mnt/TVBOXIMG/tvbox-installer/
    if [[ -f /mnt/TVBOXIMG/tvbox-installer/common.sh ]]; then
      cp -a /mnt/TVBOXIMG/tvbox-installer/*.sh /mnt/TVBOXIMG/tvbox-installer/scripts/ 2>/dev/null || true
    fi
  fi
  find /mnt/TVBOXIMG -name '*.sh' -exec chmod +x {} +
  find /mnt/TVBOXIMG -name '*.sh' -exec sed -i 's/\r$//' {} +
  df -h /mnt/TVBOXIMG
  ls -la /mnt/TVBOXIMG/tvbox-installer/scripts/ | head -30
  sync
  umount /mnt/TVBOXIMG
else
  echo "WARN: za malo miejsca na TVBOXIMG"
fi

echo "=== Final ==="
lsblk -o NAME,SIZE,TYPE,LABEL,FSTYPE "$USB"
echo "ISO_USB_DONE"
