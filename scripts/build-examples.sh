#!/usr/bin/env bash
# Compiles every example with the board settings from docs/getting-started.md.
# Usage: scripts/build-examples.sh [port]   (with a port, the examples are also uploaded)
set -euo pipefail

FQBN='esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,FlashSize=16M,FlashMode=qio,PSRAM=enabled,PartitionScheme=app3M_fat9M_16MB'
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

for sketch in "$ROOT"/examples/*/; do
  echo "==> $(basename "$sketch")"
  arduino-cli compile --fqbn "$FQBN" "$sketch"
  if [[ $# -ge 1 ]]; then
    arduino-cli upload --fqbn "$FQBN" -p "$1" "$sketch"
  fi
done
