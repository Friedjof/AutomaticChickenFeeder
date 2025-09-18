#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
DATA_TEMPLATE="${REPO_ROOT}/data-template"
DATA_DIR="${REPO_ROOT}/data"
PIO_HOME="${REPO_ROOT}/.pio-home"

info() { printf "[INFO] %s\n" "$*"; }
warn() { printf "[WARN] %s\n" "$*"; }
error() { printf "[ERROR] %s\n" "$*" >&2; }

info "Running repository setup from ${REPO_ROOT}"

if ! command -v pio >/dev/null 2>&1; then
  error "PlatformIO CLI (pio) not found. Install instructions: https://docs.platformio.org/en/latest/core/installation.html"
  exit 1
fi

mkdir -p "${PIO_HOME}"
info "Ensured local PlatformIO home at ${PIO_HOME}"

if [ ! -d "${DATA_TEMPLATE}" ]; then
  warn "${DATA_TEMPLATE} is missing; cannot scaffold data directory."
else
  if [ -d "${DATA_DIR}" ]; then
    info "${DATA_DIR} already exists; skipping copy."
  else
    cp -R "${DATA_TEMPLATE}" "${DATA_DIR}"
    info "Created ${DATA_DIR} from template. Review config.json before flashing."
  fi
fi

info "Attempting to prefetch PlatformIO packages (may require network)…"
if PLATFORMIO_HOME_DIR="${PIO_HOME}" pio pkg update --only-platform --only-library -e esp32c6 >/dev/null 2>&1; then
  info "PlatformIO packages updated."
else
  warn "Could not update PlatformIO packages. This is safe to ignore if offline; rerun after gaining network access."
fi

info "Setup complete. Next steps:"
info "  1. Edit data/config.json to match your hardware."
info "  2. Run 'make build' (or 'make build ENV=esp32c6') once dependencies are installed."
info "  3. Use 'make upload -e esp32c6' to flash the device when ready."
