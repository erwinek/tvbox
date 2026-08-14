#!/usr/bin/env bash
# TVBox USB installer menu — boot z pena, 2 opcje.
set -euo pipefail

INSTALL_ROOT="${INSTALL_ROOT:-/opt/tvbox-installer}"
# shellcheck source=common.sh
source "${INSTALL_ROOT}/scripts/common.sh"

require_root
export DEBIAN_FRONTEND=noninteractive

PAYLOAD_APP="${INSTALL_ROOT}/payload/app"
PAYLOAD_OS="${INSTALL_ROOT}/payload/os"

while true; do
  CHOICE=$(whiptail --title "TVBox Installer" --menu "Wybierz operacje (dysk docelowy = Wyse):" 16 70 4 \
    "1" "Zaktualizuj aplikacje tvbox (z USB -> dysk Wyse)" \
    "2" "Zainstaluj / odswiez CALY system (WYMAZE dysk Wyse)" \
    "3" "Shell (wyjscie do konsoli)" \
    "4" "Reboot" \
    3>&1 1>&2 2>&3) || true

  case "${CHOICE:-}" in
    1)
      if [[ ! -x "${PAYLOAD_APP}/bin/tvbox_gui" && ! -f "${PAYLOAD_APP}/bin/tvbox_gui" ]]; then
        whiptail --msgbox "Brak payload/app na USB.\nSkopiuj build apki do:\n${PAYLOAD_APP}/" 12 60
        continue
      fi
      if whiptail --yesno "Skopiowac aplikacje z USB na dysk wewnetrzny Wyse?" 10 60; then
        bash "${INSTALL_ROOT}/scripts/tvbox-install-app.sh" || \
          whiptail --msgbox "Blad instalacji apki — zobacz komunikaty na konsoli.\nEnter = wroc do menu." 12 60
        read -r -p "Enter..." _
      fi
      ;;
    2)
      if [[ ! -f "${PAYLOAD_OS}/current/root.tar.gz" && ! -f "${PAYLOAD_OS}/root.tar.gz" ]]; then
        # tez sprawdz symlink images layout
        if [[ ! -f "${INSTALL_ROOT}/payload/os/current/root.tar.gz" ]]; then
          whiptail --msgbox "Brak payload/os (capture golden).\nNajpierw: prepare/capture z golden na USB." 12 60
          continue
        fi
      fi
      if whiptail --yesno "UWAGA: WYMAZE CALY dysk wewnetrzny Wyse i wgra OS z USB.\nKontynuowac?" 12 60; then
        export TVBOX_ASSUME_YES=1
        export TVBOX_PAYLOAD_OS="${PAYLOAD_OS}"
        echo
        echo "Instalacja OS startuje. Log: /var/log/tvbox-install-os.log"
        echo "Koniec = komunikat INSTALL OK albo INSTALL FAILED (nie zostawiaj na vgcreate)."
        if bash "${INSTALL_ROOT}/scripts/tvbox-install-os.sh"; then
          whiptail --msgbox "OS zainstalowany (INSTALL OK).\nWyjmij USB i reboot.\nLog: /var/log/tvbox-install-os.log" 12 60 || true
        else
          whiptail --msgbox "BLAD instalacji (INSTALL FAILED).\nLog: /var/log/tvbox-install-os.log\nOstatnie linie na konsoli powyzej." 12 62 || true
        fi
      fi
      ;;
    3)
      whiptail --msgbox "Wychodze do shell. Menu ponownie: sudo tvbox-menu" 8 50
      exit 0
      ;;
    4)
      reboot
      ;;
    *)
      ;;
  esac
done
