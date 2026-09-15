#!/usr/bin/env bash
# Upgrade TVBox z pendrive: TVBOX_UPDATE/ -> /home/boxer/tvbox/data/app/current
# Zapis tylko na RW LV (overlayroot nie jest ruszany).
set -euo pipefail

LOG="/home/boxer/tvbox/data/usb-upgrade.log"
DEST="/home/boxer/tvbox/data/app/current"
LOCK="/run/tvbox-usb-upgrade.lock"
MARKER="TVBOX_UPDATE"

log() {
  mkdir -p "$(dirname "$LOG")"
  echo "$(date -Is) $*" | tee -a "$LOG"
}

is_usb_partition() {
  local dev="$1" sys parent
  [[ -b "$dev" ]] || return 1
  # Nie ruszaj dysku systemowego (LVM ubuntu-vg).
  if lsblk -no PKNAME "$dev" 2>/dev/null | grep -qx 'sda'; then
    return 1
  fi
  parent="$(lsblk -no PKNAME "$dev" 2>/dev/null | head -1)"
  [[ -n "$parent" ]] || parent="$(basename "$dev" | sed 's/[0-9]*$//')"
  sys="/sys/block/${parent}/removable"
  [[ -f "$sys" && "$(cat "$sys")" == "1" ]] && return 0
  lsblk -no TRAN "/dev/${parent}" 2>/dev/null | grep -qi usb
}

parse_version() {
  local f="$1" v=""
  [[ -f "$f" ]] || { echo "0"; return; }
  v="$(sed -n 's/^[[:space:]]*version=//p' "$f" | head -1 | tr -d '[:space:]')"
  [[ -n "$v" ]] || v="$(head -1 "$f" | tr -d '[:space:]')"
  [[ -n "$v" ]] || v="0"
  echo "$v"
}

version_gt() {
  # true gdy $1 > $2 (sort -V)
  local a="$1" b="$2"
  [[ "$a" == "$b" ]] && return 1
  [[ "$(printf '%s\n%s\n' "$a" "$b" | sort -V | tail -1)" == "$a" ]]
}

find_payload() {
  local root="$1"
  if [[ -d "$root/$MARKER" && -f "$root/$MARKER/bin/tvbox_gui" ]]; then
    echo "$root/$MARKER"
    return 0
  fi
  if [[ -f "$root/bin/tvbox_gui" && -f "$root/VERSION" ]]; then
    echo "$root"
    return 0
  fi
  return 1
}

upgrade_from_mount() {
  local mnt="$1" src usb_ver cur_ver
  src="$(find_payload "$mnt")" || return 1
  [[ -x "$src/bin/tvbox_gui" || -f "$src/bin/tvbox_gui" ]] || return 1

  usb_ver="$(parse_version "$src/VERSION")"
  cur_ver="$(parse_version "$DEST/VERSION")"
  log "found payload=$src usb=$usb_ver current=$cur_ver"

  if [[ "$usb_ver" == "$cur_ver" ]]; then
    log "same version $usb_ver — skip"
    return 0
  fi
  if ! version_gt "$usb_ver" "$cur_ver"; then
    log "usb $usb_ver not newer than $cur_ver — skip"
    return 0
  fi

  log "installing $usb_ver -> $DEST"
  systemctl stop tvbox.service 2>/dev/null || true
  sleep 1
  mkdir -p "$DEST/bin" "$DEST/config"
  cp -a "$src/bin/tvbox_gui" "$DEST/bin/tvbox_gui"
  chmod +x "$DEST/bin/tvbox_gui"
  if [[ -f "$src/config/app-wyse.yaml" ]]; then
    cp -a "$src/config/app-wyse.yaml" "$DEST/config/app-wyse.yaml"
  fi
  cat >"$DEST/VERSION" <<EOF
version=$usb_ver
updated=$(date -u -Iseconds)
source=usb
EOF
  chown -R boxer:boxer "$DEST" 2>/dev/null || true
  sync
  systemctl start tvbox.service 2>/dev/null || true
  log "upgrade OK $usb_ver"
}

process_dev() {
  local dev="$1" mnt
  is_usb_partition "$dev" || {
    log "ignore $dev (not usb partition)"
    return 0
  }
  mnt="$(mktemp -d /tmp/tvbox-usb.XXXXXX)"
  if ! mount -o ro,noexec,nosuid,nodev "$dev" "$mnt" 2>>"$LOG"; then
    log "mount failed $dev"
    rmdir "$mnt" 2>/dev/null || true
    return 0
  fi
  if find_payload "$mnt" >/dev/null; then
    upgrade_from_mount "$mnt" || log "upgrade failed from $dev"
  else
    log "no $MARKER on $dev"
  fi
  umount "$mnt" 2>/dev/null || umount -l "$mnt" 2>/dev/null || true
  rmdir "$mnt" 2>/dev/null || true
}

exec 9>"$LOCK"
if ! flock -n 9; then
  echo "$(date -Is) already running" >>"$LOG"
  exit 0
fi

if [[ "${1:-}" == "--scan" ]]; then
  log "scan all usb partitions"
  lsblk -ln -o NAME,TYPE,TRAN,RM | while read -r name type tran rm; do
    [[ "$type" == "part" ]] || continue
    process_dev "/dev/$name"
  done
  exit 0
fi

if [[ -n "${1:-}" ]]; then
  dev="$1"
  [[ "$dev" == /dev/* ]] || dev="/dev/$dev"
  # udev potrafi odpalic zanim partycja gotowa
  for _ in 1 2 3 4 5 6 7 8; do
    [[ -b "$dev" ]] && break
    sleep 0.5
  done
  process_dev "$dev"
  exit 0
fi

log "usage: $0 <device|--scan>"
exit 1
