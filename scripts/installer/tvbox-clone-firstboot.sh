#!/usr/bin/env bash
# First boot po klonie: unikalny hostname, machine-id, SSH host keys.
set -euo pipefail

FLAG="/home/boxer/tvbox/data/.tvbox-needs-firstboot"
[[ -f "$FLAG" ]] || exit 0

echo "[tvbox-firstboot] Running..."

# machine-id
if command -v systemd-machine-id-setup >/dev/null 2>&1; then
  systemd-machine-id-setup
else
  cat /proc/sys/kernel/random/uuid | tr -d '-' > /etc/machine-id
fi
ln -sf /etc/machine-id /var/lib/dbus/machine-id 2>/dev/null || true

# SSH host keys
rm -f /etc/ssh/ssh_host_*
ssh-keygen -A

# Hostname z MAC (pierwszy nie-loopback)
MAC=""
for f in /sys/class/net/*/address; do
  [[ -f "$f" ]] || continue
  iface="$(basename "$(dirname "$f")")"
  [[ "$iface" == "lo" ]] && continue
  addr="$(tr -d ':' <"$f")"
  [[ "$addr" == "000000000000" ]] && continue
  MAC="${addr: -4}"
  break
done
[[ -n "$MAC" ]] || MAC="$(printf '%04x' "$RANDOM")"
NEW_HOST="wyse-${MAC}"
hostnamectl set-hostname "$NEW_HOST" 2>/dev/null || echo "$NEW_HOST" > /etc/hostname
if grep -qE '^127\.0\.1\.1' /etc/hosts 2>/dev/null; then
  sed -i "s/^127\\.0\\.1\\.1.*/127.0.1.1\t${NEW_HOST}/" /etc/hosts
else
  echo -e "127.0.1.1\t${NEW_HOST}" >> /etc/hosts
fi

rm -f "$FLAG"
echo "[tvbox-firstboot] Done: hostname=$NEW_HOST"
