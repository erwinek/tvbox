#!/bin/bash
# Force 1920x1080@60 on Wyse installer/kiosk (TVs often reject 4K timing).
set -euo pipefail
MODE="${1:-1920x1080@60}"
CONN=""
for c in /sys/class/drm/card*-*; do
  [ -f "$c/status" ] || continue
  [ "$(cat "$c/status")" = connected ] || continue
  CONN="$(basename "$c")"
  break
done
# Kernel video= uses connector name without cardN- prefix, e.g. DP-1
VIDEO_ARG=""
if [ -n "$CONN" ]; then
  NAME="${CONN#card*-}"
  VIDEO_ARG="video=${NAME}:${MODE}"
fi
echo "CONNECTED=$CONN VIDEO_ARG=$VIDEO_ARG FB=$(cat /sys/class/graphics/fb0/virtual_size 2>/dev/null || echo none)"

# Persist in GRUB
GRUB=/etc/default/grub
if [ -f "$GRUB" ]; then
  sed -i 's/^GRUB_GFXMODE=.*/GRUB_GFXMODE=1920x1080/' "$GRUB"
  grep -q '^GRUB_GFXMODE=' "$GRUB" || echo 'GRUB_GFXMODE=1920x1080' >>"$GRUB"
  sed -i 's/^#GRUB_GFXMODE=.*/GRUB_GFXMODE=1920x1080/' "$GRUB"
  grep -q '^GRUB_GFXPAYLOAD_LINUX=' "$GRUB" || echo 'GRUB_GFXPAYLOAD_LINUX=keep' >>"$GRUB"
  # inject video= into CMDLINE_LINUX
  if [ -n "$VIDEO_ARG" ]; then
    if grep -q '^GRUB_CMDLINE_LINUX=' "$GRUB"; then
      if grep -q "video=" "$GRUB"; then
        sed -i -E "s/video=[^ \"']+/${VIDEO_ARG}/" "$GRUB"
      else
        sed -i -E "s|^GRUB_CMDLINE_LINUX=\"(.*)\"|GRUB_CMDLINE_LINUX=\"\1 ${VIDEO_ARG}\"|" "$GRUB"
        sed -i -E 's|^GRUB_CMDLINE_LINUX=" +(.*)"|GRUB_CMDLINE_LINUX="\1"|' "$GRUB"
      fi
    else
      echo "GRUB_CMDLINE_LINUX=\"${VIDEO_ARG}\"" >>"$GRUB"
    fi
  fi
  update-grub
fi

# Try live fb mode (may or may not take without reboot)
if command -v fbset >/dev/null; then
  fbset -g 1920 1080 1920 1080 32 || true
fi
echo "DONE grub:"
grep -E 'GRUB_GFX|GRUB_CMDLINE' /etc/default/grub
echo "FB_NOW=$(cat /sys/class/graphics/fb0/virtual_size 2>/dev/null || echo none)"
