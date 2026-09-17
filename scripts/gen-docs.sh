#!/usr/bin/env bash
# Regenerates the Luma API reference under docs/api/ from the doc comments in
# src/. Output is deterministic, so re-running this after source changes should
# only touch the modules that actually changed.
#
# Usage:
#   scripts/gen-docs.sh [path-to-compiler]
#
# If no compiler is given, uses bin/luma (run scripts/bootstrap-build.sh first).
set -euo pipefail

cd "$(dirname "$0")/.."

COMPILER="${1:-}"
if [ -z "$COMPILER" ]; then
  if [ ! -x "bin/luma" ]; then
    echo "error: bin/luma not found — run scripts/bootstrap-build.sh first," >&2
    echo "       or pass a compiler explicitly: scripts/gen-docs.sh <compiler>" >&2
    exit 1
  fi
  COMPILER="bin/luma"
fi

source "scripts/lib-src-files.sh"

"$COMPILER" src/main.lx -l "${SRC_FILES[@]}" -docs docs/api
