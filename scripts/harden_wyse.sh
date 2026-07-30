#!/usr/bin/env bash
# Hardening Ubuntu Server na Wyse pod twarde power-cycle (utrata zasilania).
# Idempotentny — bezpiecznie odpalac wielokrotnie.
set -euo pipefail

if [[ "$(id -u)" -ne 0 ]]; then
  echo "Uruchom przez: sudo bash $0"
  exit 1
fi

echo "=== TVBox Wyse OS hardening (power-cycle) ==="

backup() {
  local f="$1"
  if [[ -f "$f" && ! -f "${f}.tvbox.bak" ]]; then
    cp -a "$f" "${f}.tvbox.bak"
  fi
}

echo ""
echo "--- [1/8] fstab: noatime + errors=remount-ro ---"
backup /etc/fstab
# / (ext4 LVM)
if grep -qE '^[^#].*\s/\s+ext4\s' /etc/fstab; then
  sed -i -E 's|^([^#]\S+\s+/\s+ext4\s+)[^[:space:]]+|\1defaults,noatime,nodiratime,commit=60,errors=remount-ro|' /etc/fstab
fi
# /boot
if grep -qE '^[^#].*\s/boot\s+ext4\s' /etc/fstab; then
  sed -i -E 's|^([^#]\S+\s+/boot\s+ext4\s+)[^[:space:]]+|\1defaults,noatime,nodiratime,commit=60,errors=remount-ro|' /etc/fstab
fi
# EFI: mniej metadanych
if grep -qE '^[^#].*\s/boot/efi\s+vfat\s' /etc/fstab; then
  sed -i -E 's|^([^#]\S+\s+/boot/efi\s+vfat\s+)[^[:space:]]+|\1umask=0077,noatime|' /etc/fstab
fi
findmnt -no OPTIONS / || true
echo "fstab updated (aktywne po remount/reboot)"

echo ""
echo "--- [2/8] ext4: fsck po brudnym umount ---"
ROOT_DEV="$(findmnt -no SOURCE /)"
BOOT_DEV="$(findmnt -no SOURCE /boot 2>/dev/null || true)"
for dev in "$ROOT_DEV" "$BOOT_DEV"; do
  [[ -n "$dev" && -b "$dev" ]] || continue
  # -c 1: sprawdzaj co 1 mount gdy nieczysty; -i 1d: max 1 dzien miedzy checkami
  tune2fs -c 2 -i 1d "$dev" >/dev/null 2>&1 || true
  echo "tune2fs: $dev"
done

echo ""
echo "--- [3/8] systemd-journald: limit zapisu ---"
backup /etc/systemd/journald.conf
mkdir -p /etc/systemd/journald.conf.d
cat >/etc/systemd/journald.conf.d/99-tvbox.conf <<'EOF'
[Journal]
Storage=persistent
SystemMaxUse=100M
SystemMaxFileSize=20M
RuntimeMaxUse=50M
MaxRetentionSec=7day
SyncIntervalSec=60s
EOF
systemctl restart systemd-journald 2>/dev/null || true

echo ""
echo "--- [4/8] sysctl: panic reboot + mniejsze dirty writeback ---"
cat >/etc/sysctl.d/99-tvbox-kiosk.conf <<'EOF'
# Po kernel panic — reboot zamiast zawieszenia (kiosk).
kernel.panic = 10
kernel.panic_on_oops = 1
kernel.sysrq = 0

# Szybciej zrzucaj dirty pages na dysk — mniej utraty przy power-cut.
vm.dirty_background_ratio = 5
vm.dirty_ratio = 10
vm.dirty_expire_centisecs = 1500
vm.dirty_writeback_centisecs = 500

# Mniej swap thrashu na slabym Celeronie + malej RAM.
vm.swappiness = 10
EOF
sysctl --system >/dev/null 2>&1 || sysctl -p /etc/sysctl.d/99-tvbox-kiosk.conf

echo ""
echo "--- [5/8] Wylacz suspend / hibernate ---"
systemctl mask sleep.target suspend.target hibernate.target hybrid-sleep.target 2>/dev/null || true
# logind: ignoruj przycisk zasilania (opcjonalnie — zostaw poweroff)
mkdir -p /etc/systemd/logind.conf.d
cat >/etc/systemd/logind.conf.d/99-tvbox.conf <<'EOF'
[Login]
HandleLidSwitch=ignore
HandleLidSwitchExternalPower=ignore
HandleLidSwitchDocked=ignore
IdleAction=ignore
EOF
systemctl restart systemd-logind 2>/dev/null || true

echo ""
echo "--- [6/8] Brak auto-apt w trakcie dzialania (ryzyko power-cut mid-dpkg) ---"
if systemctl list-unit-files unattended-upgrades.service >/dev/null 2>&1; then
  systemctl disable --now unattended-upgrades.service 2>/dev/null || true
  systemctl mask unattended-upgrades.service 2>/dev/null || true
  echo "unattended-upgrades: disabled+masked"
fi
# apt daily timers
systemctl disable --now apt-daily.timer apt-daily-upgrade.timer 2>/dev/null || true

echo ""
echo "--- [7/8] Boot: fsck repair + uslugi kiosk ---"
# GRUB: naprawa FS przy starcie
if [[ -f /etc/default/grub ]]; then
  backup /etc/default/grub
  if grep -q '^GRUB_CMDLINE_LINUX_DEFAULT=' /etc/default/grub; then
    # Dodaj fsck.repair=yes i fsck.mode=auto jesli brak
    if ! grep -q 'fsck.repair=' /etc/default/grub; then
      sed -i 's|^GRUB_CMDLINE_LINUX_DEFAULT="\(.*\)"|GRUB_CMDLINE_LINUX_DEFAULT="\1 fsck.mode=auto fsck.repair=yes"|' /etc/default/grub
    fi
  fi
  update-grub 2>/dev/null || true
fi

# systemd: dluzszy timeout startu po fsck
mkdir -p /etc/systemd/system.conf.d
cat >/etc/systemd/system.conf.d/99-tvbox.conf <<'EOF'
[Manager]
DefaultTimeoutStartSec=120s
DefaultTimeoutStopSec=30s
EOF
systemctl daemon-reexec 2>/dev/null || systemctl daemon-reload

# Kiosk services
systemctl enable seatd.service 2>/dev/null || true
systemctl enable tvbox.service 2>/dev/null || true
systemctl disable getty@tty1.service 2>/dev/null || true

echo ""
echo "--- [8/8] tmpfs /tmp + ograniczenie zapisu logow tvbox ---"
# /tmp w RAM — mniej I/O na root przy tempach
if ! grep -qE '^tmpfs\s+/tmp\s' /etc/fstab; then
  echo 'tmpfs /tmp tmpfs defaults,noatime,mode=1777,size=256M 0 0' >> /etc/fstab
  echo "dodano tmpfs /tmp"
fi

# Rotacja logu aplikacji (jesli plikowy)
mkdir -p /etc/logrotate.d
cat >/etc/logrotate.d/tvbox <<'EOF'
/home/boxer/tvbox/data/tvbox.log {
    size 5M
    rotate 3
    missingok
    notifempty
    compress
    copytruncate
}
EOF

# Remount root z nowymi opcjami (best-effort, bez reboot)
mount -o remount,noatime,nodiratime,commit=60,errors=remount-ro / 2>/dev/null \
  && echo "root remounted with noatime/commit=60" \
  || echo "root remount skipped (OK — zadziala po reboot)"

echo ""
echo "=== Hardening complete ==="
echo "Zalecany restart: sudo reboot"
echo "Po power-cycle: fsck (auto), tvbox+seatd, brak suspend/apt-daily."
echo "Sprawdz: systemctl is-enabled tvbox seatd; findmnt /"
