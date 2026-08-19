#!/usr/bin/env bash
# tvbox-update-watcher.sh — automatyczna aktualizacja apki z pendrive.
# Uruchamiany przez udev (ExecStart w tvbox-update-watcher.service).
#
# Pendrive musi miec LABEL=TVBOX-UPDATE i zawierat:
#   version.txt          — jedna linia: "1.4.2"
#   bin/tvbox_gui        — binarka (opcjonalnie)
#   config/app-wyse.yaml — config (opcjonalnie)
#   assets/              — fonty, dzwieki itp. (opcjonalnie)
#   backgrounds/         — klipy tla (opcjonalnie)
#
# Strategia: payload -> staging dir (atomiczny) -> restart tvbox.

set -euo pipefail

LOG_FILE="/home/boxer/tvbox/data/tvbox-update.log"
INSTALL_DIR="/home/boxer/tvbox"
DATA_DIR="$INSTALL_DIR/data"
USB_LABEL="TVBOX-UPDATE"
USB_MNT="/mnt/tvbox-update-usb"
STAGING_BASE="$DATA_DIR/app"
VERSION_FILE="$INSTALL_DIR/VERSION"

log() {
    local msg="[$(date -u -Iseconds)] $*"
    echo "$msg"
    echo "$msg" >> "$LOG_FILE" 2>/dev/null || true
}

die() {
    log "ERROR: $*"
    exit 1
}

cleanup() {
    if mountpoint -q "$USB_MNT" 2>/dev/null; then
        umount "$USB_MNT" 2>/dev/null || true
    fi
}
trap cleanup EXIT

# --- Znajdz pendrive po labelu ---
log "=== tvbox-update-watcher start ==="
USB_DEV=""
for i in $(seq 1 10); do
    USB_DEV="$(blkid -L "$USB_LABEL" 2>/dev/null || true)"
    [[ -n "$USB_DEV" ]] && break
    log "Czekam na $USB_LABEL ($i/10)..."
    sleep 2
done

[[ -n "$USB_DEV" ]] || die "Nie znaleziono urzadzenia z LABEL=$USB_LABEL"
log "Pendrive: $USB_DEV"

# --- Montowanie ---
mkdir -p "$USB_MNT"
mount -o ro "$USB_DEV" "$USB_MNT" || die "Nie mozna zamontowac $USB_DEV"
log "Zamontowano $USB_DEV -> $USB_MNT"

USB_VERSION_FILE="$USB_MNT/version.txt"
[[ -f "$USB_VERSION_FILE" ]] || die "Brak version.txt na pendrive"

USB_VERSION="$(cat "$USB_VERSION_FILE" | tr -d '[:space:]')"
[[ -n "$USB_VERSION" ]] || die "version.txt jest pusty"

# --- Porownaj wersje ---
CURRENT_VERSION=""
[[ -f "$VERSION_FILE" ]] && CURRENT_VERSION="$(cat "$VERSION_FILE" | tr -d '[:space:]')"
log "Wersja biezaca: '${CURRENT_VERSION:-brak}' | Wersja na pendrive: '$USB_VERSION'"

if [[ "$USB_VERSION" == "$CURRENT_VERSION" ]]; then
    log "Wersje identyczne — brak aktualizacji."
    umount "$USB_MNT"
    exit 0
fi

# --- Kopiowanie do staging ---
STAGING="$STAGING_BASE/$USB_VERSION"
log "Kopiuje do $STAGING ..."
mkdir -p "$STAGING_BASE"
rm -rf "$STAGING.tmp"
mkdir -p "$STAGING.tmp"

# Kopiuj tylko to co istnieje na pendrive (nie nadpisuj brakujacych elementow)
for item in bin config assets backgrounds; do
    [[ -d "$USB_MNT/$item" ]] && rsync -a "$USB_MNT/$item/" "$STAGING.tmp/$item/"
done
for item in version.txt; do
    [[ -f "$USB_MNT/$item" ]] && cp "$USB_MNT/$item" "$STAGING.tmp/$item"
done

# --- Weryfikacja binarki (jesli byla na pendrive) ---
if [[ -f "$STAGING.tmp/bin/tvbox_gui" ]]; then
    chmod +x "$STAGING.tmp/bin/tvbox_gui"
    if ! file "$STAGING.tmp/bin/tvbox_gui" | grep -qiE 'ELF|executable'; then
        die "bin/tvbox_gui nie jest wykonywalnym plikiem ELF"
    fi
    log "Binarka OK: $(file "$STAGING.tmp/bin/tvbox_gui")"
fi

# --- Atomic: rename staging -> final ---
mv "$STAGING.tmp" "$STAGING"
log "Staging gotowy: $STAGING"

# Symlink current -> nowa wersja
ln -sfn "$STAGING" "$STAGING_BASE/current"
log "Symlink: $STAGING_BASE/current -> $STAGING"

# Zaktualizuj VERSION w INSTALL_DIR
echo "$USB_VERSION" > "$VERSION_FILE"

# --- Uzupelnij INSTALL_DIR o nowe pliki jesli symlink nie dziala (fallback) ---
# (kiosk-run.sh szuka najpierw data/app/current, potem /home/boxer/tvbox)
if [[ -f "$STAGING/bin/tvbox_gui" ]]; then
    mkdir -p "$INSTALL_DIR/bin"
    cp -a "$STAGING/bin/tvbox_gui" "$INSTALL_DIR/bin/tvbox_gui"
    chmod +x "$INSTALL_DIR/bin/tvbox_gui"
fi
for f in config/app-wyse.yaml config/sway-kiosk.conf; do
    if [[ -f "$STAGING/$f" ]]; then
        mkdir -p "$INSTALL_DIR/$(dirname $f)"
        cp -a "$STAGING/$f" "$INSTALL_DIR/$f"
    fi
done

sync
log "Sync OK."

# --- Odmontuj pendrive (lampka USB gaśnie = sygnal dla serwisanta) ---
umount "$USB_MNT"
log "Pendrive odmontowany — mozna wyjac."

# --- Restart aplikacji ---
log "Restartuje tvbox.service ..."
systemctl restart tvbox.service
log "=== Aktualizacja do $USB_VERSION zakonczona pomyslnie ==="
