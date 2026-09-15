#!/usr/bin/env bash
# Zapisz paczke TVBOX_UPDATE na pierwszym pendrive USB (uruchamiaj na Wyse).
set -euo pipefail

if [[ "$(id -u)" -ne 0 ]]; then
  echo "Uruchom: sudo bash $0"
  exit 1
fi

KIT="/home/boxer/tvbox/data/upgrade-kit/TVBOX_UPDATE"
[[ -f "$KIT/bin/tvbox_gui" ]] || { echo "Brak $KIT/bin/tvbox_gui — najpierw zloz kit."; exit 1; }

USB=""
while read -r name type tran; do
  [[ "$type" == "disk" && "${tran,,}" == "usb" ]] || continue
  USB="/dev/$name"
  break
done < <(lsblk -ln -o NAME,TYPE,TRAN)

[[ -n "$USB" ]] || { echo "Brak dysku USB. Wloz pendrive w Wyse i sprobuj ponownie."; exit 1; }
echo "USB: $USB"
lsblk "$USB"

PART=""
while read -r name type; do
  [[ "$type" == "part" ]] || continue
  PART="/dev/$name"
  break
done < <(lsblk -ln -o NAME,TYPE "$USB")

[[ -n "$PART" ]] || { echo "Brak partycji na $USB (sformatuj FAT32)."; exit 1; }

MNT="/mnt/tvbox-upgrade-usb"
mkdir -p "$MNT"
umount "$PART" 2>/dev/null || true
mount "$PART" "$MNT"
mkdir -p "$MNT/TVBOX_UPDATE"
rsync -a --delete "$KIT"/ "$MNT/TVBOX_UPDATE"/
sync
echo "Zapisano:"
find "$MNT/TVBOX_UPDATE" -type f | head
umount "$MNT"
echo "OK — wyjmij pendrive. U klienta wloz w USB Dell Wyse (thin client), nie w monitor."
