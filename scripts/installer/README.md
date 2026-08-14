# TVBox USB installer appliance

Ubuntu Server na pendrive (hostname `boxerinstaller`). Boot z USB → menu na tty1.

## Menu

1. **Aktualizacja apki** — `payload/app` → `tvbox-data:/app/current`
2. **Instalacja OS** — wipe eMMC, restore `payload/os/current/*.tar.gz`
3. Shell
4. Reboot

Kiosk na produkcji czyta apkę z `/home/boxer/tvbox/data/app/current` (overlayroot).

## Layout target (~16 GB eMMC)

| Part / LV | Size |
|-----------|------|
| EFI (`mmcblk0p1`) | 256M |
| boot (`mmcblk0p2`) | 768M |
| `tvbox-vg/ubuntu-lv` | 10G |
| `tvbox-vg/tvbox-data` | 3G |

Installer USB sam ma LVM `ubuntu-vg`, więc klon **nie może** użyć tej nazwy — target to `tvbox-vg`.

## Capture golden → payload (z PC)

Golden musi być w sieci (`ssh wyse`).

```powershell
scp scripts\installer\capture_golden_local.sh wyse:/tmp/
ssh wyse "sudo bash /tmp/capture_golden_local.sh /home/boxer/tvbox/data/.capture-out"
# potem skopiuj golden-*/{root,boot,efi,data}.tar.gz na installer:
#   /opt/tvbox-installer/payload/os/current/
# apka: rsync /home/boxer/tvbox/ (bez data/) → payload/app/
```

## Deploy skryptów na USB

```powershell
# z C:\github\tvbox — USB musi być zbottowany (SSH boxer@IP)
scp scripts\installer\*.sh boxer@<installer>:/tmp/tvbox-scripts/
ssh boxer@<installer> "sudo cp /tmp/tvbox-scripts/*.sh /opt/tvbox-installer/scripts/; sudo chmod +x /opt/tvbox-installer/scripts/*.sh"
```

Pierwszy setup OS na penie: `setup_appliance.sh` (whiptail, NOPASSWD, usługa menu, 1080p).

## Ważne (dracut)

Initrd z golden ma wbudowane `root=/dev/mapper/ubuntu--vg-ubuntu--lv`. Bez przebudowy eMMC pada na dracut emergency (`/run/initramfs/rdsosreport.txt`).

`tvbox-install-os.sh` po extract robi `hostonly=no` + `dracut -f --regenerate-all`.

Ręczna naprawa (z USB, root):

```bash
sudo bash /opt/tvbox-installer/scripts/tvbox-fix-target-initrd.sh
```

Monitor: wymuszone `1920x1080@60` (`fix_display_1080p.sh`) — 4K z DP daje „input timing is not supported”.

## Pliki

| Skrypt | Rola |
|--------|------|
| `setup_appliance.sh` | Raz: pakiety, menu systemd, sudo, 1080p |
| `tvbox-installer-menu.sh` + `.service` | Menu tty1 |
| `tvbox-install-os.sh` | Opcja 2 — OS na eMMC |
| `tvbox-install-app.sh` | Opcja 1 — apka na data LV |
| `tvbox-clone-partition.sh` | GPT + LVM `tvbox-vg` |
| `capture_golden_local.sh` | Capture z overlayroot golden |
| `tvbox-fix-target-initrd.sh` | Naprawa dracut po klonie |
| `fix_display_1080p.sh` | GRUB 1080p |
| `common.sh` | Wykrywanie dysków (USB vs mmc) |
