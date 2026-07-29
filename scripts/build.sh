#!/usr/bin/env bash
# build.sh -- convenience wrapper around `make` for hl2-nx.
#
# Usage:
#   ./scripts/build.sh           # release build (default)
#   ./scripts/build.sh debug     # -O0 -g, no LTO, easier debugging
#   ./scripts/build.sh clean     # make clean
#   ./scripts/build.sh run       # build + show install instructions
#
# Requires devkitPro installed with DEVKITPRO in the environment.

set -euo pipefail

cd "$(dirname "$0")/.."

if [[ -z "${DEVKITPRO:-}" ]]; then
  echo "error: DEVKITPRO is not set" >&2
  echo "  install devkitPro and: export DEVKITPRO=/opt/devkitpro" >&2
  exit 1
fi

mode="${1:-release}"

case "$mode" in
  release)
    make -j"$(nproc 2>/dev/null || echo 4)"
    ;;
  debug)
    # Override the optimisation flags from the Makefile. We still need
    # -fPIE -march=armv8-a+... -mtp=soft for the Switch ABI.
    make -j"$(nproc 2>/dev/null || echo 4)" \
      CFLAGS="-O0 -g -Wall -Wextra -fPIE -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -D__SWITCH__" \
      LDFLAGS="-specs=${DEVKITPRO}/libnx/switch.specs -g -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE -Wl,-Map,hl2_nx.map"
    ;;
  clean)
    make clean
    ;;
  run)
    ./scripts/build.sh release
    echo
    echo "==> hl2_nx.nro built. Copy it to your SD card:"
    echo "    /switch/hl2_nx/hl2_nx.nro"
    echo
    echo "==> See README.md for the full data layout."
    ;;
  *)
    echo "usage: $0 {release|debug|clean|run}" >&2
    exit 2
    ;;
esac
