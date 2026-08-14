#!/usr/bin/env bash
# Auto-restore na Ubuntu live (target). Bezpieczniki: flaga DO_RESTORE + dysk wewnetrzny.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "${SCRIPT_DIR}/common.sh"

require_root
export TVBOX_ASSUME_YES=1

# Znajdz / zamontuj Ventoy
MNT="/mnt/TVBOXIMG"
mkdir -p "$MNT"
if ! findmnt -n "$MNT" >/dev/null 2>&1; then
  mount_tvboximg "$MNT" >/dev/null
fi

FLAG="$MNT/tvbox-installer/DO_RESTORE"
DONE="$MNT/tvbox-installer/RESTORE_DONE"
[[ -f "$FLAG" ]] || {
  log "Brak flagi DO_RESTORE — nic nie robie (capture-only USB?)."
  exit 0
}
[[ -f "$DONE" ]] && {
  log "RESTORE_DONE juz istnieje — pomijam."
  exit 0
}

# Skrypty mogly byc skopiowane obok flagi
SCR="$MNT/tvbox-installer/scripts"
[[ -f "$SCR/tvbox-clone-restore.sh" ]] && SCRIPT_DIR="$SCR"

log "=== AUTO RESTORE za 15s (Ctrl+C aby anulowac) ==="
log "Dyski:"
lsblk
sleep 15

bash "$SCRIPT_DIR/tvbox-clone-restore.sh"

touch "$DONE"
rm -f "$FLAG"
sync
log "AUTO RESTORE zakonczony. Za 20s reboot — WYJmij USB."
sleep 20
reboot || systemctl reboot || true
