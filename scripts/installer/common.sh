#!/usr/bin/env bash
# Wspolne helpery dla tvbox-clone-*.sh (Ubuntu live).
set -euo pipefail

TVBOX_IMG_LABEL="${TVBOX_IMG_LABEL:-TVBOXIMG}"
EXPECTED_GOLDEN_BYTES="${EXPECTED_GOLDEN_BYTES:-32017047552}"
MIN_TARGET_BYTES=$((14 * 1024 * 1024 * 1024))  # ~14 GiB

log() { echo "[tvbox-clone] $*"; }
die() { echo "[tvbox-clone] ERROR: $*" >&2; exit 1; }

require_root() {
  [[ "$(id -u)" -eq 0 ]] || die "Uruchom jako root (sudo -i)"
}

# Lista dyskow (NAME SIZE TRAN MODEL) — bez loop/ram.
list_disks() {
  lsblk -bdn -o NAME,SIZE,TYPE,TRAN,MODEL | awk '$3=="disk"{print}'
}

# Boot media: dysk z ktorego zbootowalismy (ISO/Ventoy/USB/LVM na USB).
boot_disk() {
  local src name typ
  src="$(findmnt -n -o SOURCE /cdrom 2>/dev/null || true)"
  [[ -z "$src" ]] && src="$(findmnt -n -o SOURCE / 2>/dev/null || true)"
  [[ -n "$src" ]] || return 1
  # Idz w gore drzewa do TYPE=disk (dziala dla LVM/mapper)
  while read -r name typ; do
    if [[ "$typ" == "disk" ]]; then
      echo "$name"
      return 0
    fi
  done < <(lsblk -nlso NAME,TYPE "$src" 2>/dev/null || true)
  # Fallback: obetnij numer partycji
  src="${src#/dev/}"
  if [[ "$src" == nvme*n* || "$src" == mmcblk* ]]; then
    echo "$src" | sed -E 's/p[0-9]+$//'
  else
    echo "$src" | sed -E 's/[0-9]+$//'
  fi
}

is_skip_disk() {
  local name="$1" boot="$2" tran="$3"
  [[ "$name" == "$boot" ]] && return 0
  [[ "${tran,,}" == "usb" ]] && return 0
  # eMMC boot partitions (4M) — nigdy nie target
  [[ "$name" == mmcblk*boot* ]] && return 0
  [[ "$name" == mmcblk*rpmb* ]] && return 0
  return 1
}

# Preferuj najwiekszy dysk SATA/NVMe/eMMC nie bedacy boot USB.
pick_internal_disk() {
  local boot="$1"
  local name size type tran model
  local best="" best_size=0
  while read -r name size type tran model; do
    [[ "$type" == "disk" ]] || continue
    is_skip_disk "$name" "$boot" "$tran" && continue
    if (( size > best_size )); then
      best="/dev/$name"
      best_size=$size
    fi
  done < <(list_disks)
  [[ -n "$best" ]] || return 1
  echo "$best"
}

# Partycja N dysku (obsługa nvme/mmcblk)
disk_part() {
  local disk="$1" n="$2"
  if [[ "$disk" == *nvme* || "$disk" == *mmcblk* ]]; then
    echo "${disk}p${n}"
  else
    echo "${disk}${n}"
  fi
}

disk_size_bytes() {
  lsblk -bdn -o SIZE "$1"
}

confirm() {
  local msg="$1"
  if [[ "${TVBOX_ASSUME_YES:-0}" == "1" ]] || [[ "${TVBOX_ASSUME_YES:-}" == "YES" ]]; then
    log "AUTO-YES: $msg"
    return 0
  fi
  echo ""
  echo "=== $msg ==="
  echo -n "Wpisz YES aby kontynuowac: "
  read -r ans
  [[ "$ans" == "YES" ]] || die "Anulowano"
}

mount_tvboximg() {
  local mnt="${1:-/mnt/TVBOXIMG}"
  mkdir -p "$mnt"
  if findmnt -n "$mnt" >/dev/null 2>&1; then
    echo "$mnt"
    return 0
  fi
  local dev=""
  local lab
  for lab in Ventoy VENTOY TVBOXIMG; do
    dev="$(blkid -L "$lab" 2>/dev/null || true)"
    [[ -n "$dev" ]] && break
  done
  if [[ -z "$dev" ]]; then
    # case-insensitive by-label scan
    local cand
    for cand in /dev/disk/by-label/*; do
      [[ -e "$cand" ]] || continue
      lab="$(basename "$cand" | tr '[:upper:]' '[:lower:]')"
      if [[ "$lab" == *ventoy* ]] || [[ "$lab" == *tvbox* ]]; then
        dev="$(readlink -f "$cand")"
        break
      fi
    done
  fi
  [[ -n "$dev" ]] || die "Nie znaleziono partycji Ventoy/TVBOXIMG — zamontuj recznie"
  mount "$dev" "$mnt" || die "mount $dev $mnt failed"
  echo "$mnt"
}

images_dir() {
  local mnt="$1"
  mkdir -p "$mnt/images" "$mnt/scripts"
  echo "$mnt/images"
}
