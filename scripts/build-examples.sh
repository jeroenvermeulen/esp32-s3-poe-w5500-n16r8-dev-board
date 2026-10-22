#!/usr/bin/env bash
# Compiles every example with the board settings from docs/getting-started.md.
# Usage: scripts/build-examples.sh [port]   (with a port, the examples are also uploaded)
set -euo pipefail

FQBN='esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,FlashSize=16M,FlashMode=qio,PSRAM=enabled,PartitionScheme=app3M_fat9M_16MB'
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

for sketch in "$ROOT"/examples/*/; do
  echo "==> $(basename "$sketch")"
  # A sketch.yaml with default_fqbn overrides the board settings (usb_keyboard)
  fqbn="$FQBN"
  if [[ -f "$sketch/sketch.yaml" ]]; then
    fqbn="$(sed -n 's/^default_fqbn: *//p' "$sketch/sketch.yaml")"
  fi
  arduino-cli compile --fqbn "$fqbn" "$sketch"
  if [[ $# -ge 1 ]]; then
    if [[ "$fqbn" == "$FQBN" ]]; then
      arduino-cli upload --fqbn "$fqbn" -p "$1" "$sketch"
    else
      echo "    Not uploaded: it needs other board settings. Upload it on its own, see the sketch header."
    fi
  fi
done
