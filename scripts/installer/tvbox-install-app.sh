#!/usr/bin/env bash
# Opcja 1: skopiuj apke z USB payload/app na dysk Wyse (data/app/current).
set -euo pipefail

INSTALL_ROOT="${INSTALL_ROOT:-/opt/tvbox-installer}"
# shellcheck source=common.sh
source "${INSTALL_ROOT}/scripts/common.sh"
require_root

SRC="${INSTALL_ROOT}/payload/app"
[[ -d "$SRC" ]] || die "Brak $SRC"

BOOT="$(boot_disk || true)"
INTERNAL="$(pick_internal_disk "${BOOT:-}" || true)"
[[ -n "$INTERNAL" ]] || die "Brak dysku wewnetrznego Wyse"

log "Internal: $INTERNAL"
lsblk "$INTERNAL"

# Aktywuj LVM jesli OS juz zainstalowany
vgchange -ay 2>/dev/null || true
# Klon uzywa tvbox-vg; ubuntu-vg na USB to sam installer — nie ruszac.
DATA_LV=""
ROOT_LV=""
BOOT_SRC="$(findmnt -n -o SOURCE / 2>/dev/null || true)"
BOOT_SRC="$(readlink -f "$BOOT_SRC" 2>/dev/null || echo "$BOOT_SRC")"
for vg in tvbox-vg ubuntu-vg; do
  if [[ -z "$DATA_LV" && -b "/dev/${vg}/tvbox-data" ]]; then
    DATA_LV="/dev/${vg}/tvbox-data"
  fi
  if [[ -b "/dev/${vg}/ubuntu-lv" ]]; then
    lv_src="$(readlink -f "/dev/${vg}/ubuntu-lv" 2>/dev/null || true)"
    if [[ -n "$BOOT_SRC" && -n "$lv_src" && "$lv_src" == "$BOOT_SRC" ]]; then
      continue
    fi
    ROOT_LV="/dev/${vg}/ubuntu-lv"
  fi
done

MNT="/mnt/wyse-target"
rm -rf "$MNT"
mkdir -p "$MNT"

if [[ -b "$DATA_LV" ]]; then
  mount "$DATA_LV" "$MNT"
  DEST="$MNT/app/current"
elif [[ -b "$ROOT_LV" ]]; then
  mount "$ROOT_LV" "$MNT"
  # overlay-safe path even if data LV missing: create under home path on root
  mkdir -p "$MNT/home/boxer/tvbox/data"
  DEST="$MNT/home/boxer/tvbox/data/app/current"
else
  die "Brak LVM ubuntu-vg na $INTERNAL — najpierw opcja 2 (caly OS)"
fi

log "Kopiuje apke -> $DEST"
mkdir -p "$DEST"
rsync -a --delete \
  --exclude='data/videos' \
  --exclude='data/*.db' \
  --exclude='data/*.log' \
  "$SRC"/ "$DEST"/

# Wrapper /home/boxer/tvbox -> data/app/current gdy montujemy root
if findmnt -n "$MNT" | grep -q ubuntu--lv; then
  mkdir -p "$MNT/home/boxer/tvbox"
  # Jesli data jest osobnym LV, po boot bedzie zamontowane; tu ustawiamy current na data
  if [[ -b "$DATA_LV" ]]; then
    umount "$MNT" || true
    mkdir -p /mnt/wyse-root /mnt/wyse-data
    mount "$ROOT_LV" /mnt/wyse-root
    mount "$DATA_LV" /mnt/wyse-data
    mkdir -p /mnt/wyse-data/app/current
    rsync -a --delete "$SRC"/ /mnt/wyse-data/app/current/
    mkdir -p /mnt/wyse-root/home/boxer/tvbox
    # kiosk run script
    mkdir -p /mnt/wyse-root/home/boxer/tvbox/bin
    cat >/mnt/wyse-root/home/boxer/tvbox/bin/tvbox-kiosk-run.sh <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
APP_ROOT="/home/boxer/tvbox/data/app/current"
if [[ ! -x "$APP_ROOT/bin/tvbox_gui" ]]; then
  APP_ROOT="/home/boxer/tvbox"
fi
cd "$APP_ROOT"
export SDL_VIDEODRIVER=wayland
mkdir -p /home/boxer/tvbox/data
exec >>/home/boxer/tvbox/data/tvbox.log 2>&1
echo "=== start $(date -Is) APP_ROOT=$APP_ROOT WAYLAND_DISPLAY=${WAYLAND_DISPLAY:-} ==="
CFG=./config/app-wyse.yaml
[[ -f "$CFG" ]] || CFG=/home/boxer/tvbox/config/app-wyse.yaml
./bin/tvbox_gui "$CFG" || true
echo "=== exit $? $(date -Is) ==="
swaymsg exit 2>/dev/null || true
EOF
    chmod +x /mnt/wyse-root/home/boxer/tvbox/bin/tvbox-kiosk-run.sh
    # Symlink convenience
    ln -sfn /home/boxer/tvbox/data/app/current/bin /mnt/wyse-root/home/boxer/tvbox/bin-app 2>/dev/null || true
    if [[ -f /mnt/wyse-data/app/current/config/sway-kiosk.conf ]]; then
      mkdir -p /mnt/wyse-root/home/boxer/tvbox/config
      cp -a /mnt/wyse-data/app/current/config/sway-kiosk.conf /mnt/wyse-root/home/boxer/tvbox/config/ 2>/dev/null || true
    fi
    # Patch sway to use app current if needed — keep WorkingDirectory style via kiosk-run
    umount /mnt/wyse-data /mnt/wyse-root || true
  else
    umount "$MNT" || true
  fi
else
  # Only data LV was mounted
  mkdir -p "$MNT/app/current"
  cat >"$MNT/app/current/VERSION" <<EOF
updated=$(date -u -Iseconds)
EOF
  umount "$MNT" || true
fi

sync
log "Aplikacja zainstalowana. Wyjmij USB i zbootuj z dysku Wyse (lub restart tvbox)."
whiptail --msgbox "OK: apka skopiowana na dysk Wyse." 8 50 || true
