#!/usr/bin/env bash
# Flash ESP32 (pgm.ino.bin) z Dell Wyse przez USB programator.
# Trwale: firmware + venv esptool na /home/boxer/tvbox/data (overlayroot-safe).
set -euo pipefail

DATA_DIR="${TVBOX_DATA_DIR:-/home/boxer/tvbox/data}"
FIRMWARE_DIR="${DATA_DIR}/firmware"
FIRMWARE="${FIRMWARE:-${FIRMWARE_DIR}/pgm.ino.bin}"
PORT="${PORT:-/dev/ttyUSB0}"
CHIP="${CHIP:-esp32}"
BAUD="${BAUD:-921600}"
FLASH_ADDR="${FLASH_ADDR:-0x10000}"
SERVICE_NAME="${SERVICE_NAME:-tvbox}"
VENV_DIR="${DATA_DIR}/.venv-esptool"

resolve_esptool() {
  if [[ -x "${VENV_DIR}/bin/esptool" ]]; then
    echo "${VENV_DIR}/bin/esptool"
  elif [[ -x "${VENV_DIR}/bin/esptool.py" ]]; then
    echo "${VENV_DIR}/bin/esptool.py"
  else
    return 1
  fi
}

echo "=== Flash ESP32 (Wyse) ==="
echo "firmware: ${FIRMWARE}"
echo "port:     ${PORT}"
echo "chip:     ${CHIP} @ ${FLASH_ADDR}"

if [[ ! -f "${FIRMWARE}" ]]; then
  echo "Brak pliku firmware: ${FIRMWARE}" >&2
  exit 1
fi

if [[ ! -e "${PORT}" ]]; then
  echo "Brak portu ${PORT}" >&2
  exit 1
fi

ensure_esptool() {
  if resolve_esptool >/dev/null; then
    return 0
  fi

  echo "--- Setup esptool venv w ${VENV_DIR} ---"
  rm -rf "${VENV_DIR}"
  if ! python3 -m venv "${VENV_DIR}" 2>/dev/null; then
    echo "python3-venv niedostepny — instalacja (moze byc ephemeral przy overlayroot)..."
    sudo apt-get update -qq
    sudo apt-get install -y --no-install-recommends python3-venv python3-pip
    rm -rf "${VENV_DIR}"
    python3 -m venv "${VENV_DIR}"
  fi

  # shellcheck disable=SC1091
  source "${VENV_DIR}/bin/activate"
  pip install -q --upgrade pip
  pip install -q esptool
  deactivate

  if ! resolve_esptool >/dev/null; then
    echo "esptool nie zostal zainstalowany w venv" >&2
    exit 1
  fi
}

ensure_esptool
ESPTOOL="$(resolve_esptool)"

RESTART_TVBOX=0
if systemctl is-active --quiet "${SERVICE_NAME}.service" 2>/dev/null; then
  echo "--- Stop ${SERVICE_NAME} (zwolnij ${PORT}) ---"
  sudo systemctl stop "${SERVICE_NAME}.service"
  RESTART_TVBOX=1
  sleep 1
fi

cleanup() {
  if [[ "${RESTART_TVBOX}" -eq 1 ]]; then
    echo "--- Start ${SERVICE_NAME} ---"
    sudo systemctl start "${SERVICE_NAME}.service" || true
  fi
}
trap cleanup EXIT

echo "--- esptool write-flash ---"
"${ESPTOOL}" --chip "${CHIP}" -p "${PORT}" -b "${BAUD}" \
  write-flash -z "${FLASH_ADDR}" "${FIRMWARE}"

echo "=== Flash OK ==="
