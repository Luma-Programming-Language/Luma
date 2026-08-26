#!/usr/bin/env bash
# Cross-compiles the luma compiler ITSELF for Windows, using a native
# bin/luma (already built by bootstrap-build.sh) plus mingw-w64 as the
# actual C toolchain — see src/codegen/codegen.lx's target_cc_prefix.
#
# Windows-only: there's no equivalent real cross-compiler package for
# macOS (only osxcross, which needs Apple's SDK extracted from a real
# Xcode install). For macOS, see scripts/transpile.sh instead — it emits
# the C on any host, for an actual macOS machine's own `cc` to link.
#
# Usage:
#   scripts/cross-build.sh <windows64|windows32> <output-path>
#
# Requires: bin/luma already built, and mingw-w64 on PATH
# (x86_64-w64-mingw32-gcc / i686-w64-mingw32-gcc — package name is
# mingw-w64-gcc on Arch, gcc-mingw-w64-x86-64 on Debian/Ubuntu).
set -euo pipefail

cd "$(dirname "$0")/.."
source "scripts/lib-src-files.sh"

TARGET="${1:?usage: cross-build.sh <windows64|windows32> <output-path>}"
OUT="${2:?usage: cross-build.sh <windows64|windows32> <output-path>}"

case "$TARGET" in
  windows64) COMPILER="x86_64-w64-mingw32-gcc" ;;
  windows32) COMPILER="i686-w64-mingw32-gcc" ;;
  *) echo "error: unknown target '$TARGET' (expected windows64 or windows32 — see scripts/transpile.sh for macos)" >&2; exit 1 ;;
esac

if [ ! -x "bin/luma" ]; then
  echo "error: bin/luma not found — run scripts/bootstrap-build.sh first" >&2
  exit 1
fi
if ! command -v "$COMPILER" >/dev/null 2>&1; then
  echo "error: $COMPILER not found on PATH — required for cross-compiling to $TARGET (mingw-w64-gcc on Arch, gcc-mingw-w64-x86-64 on Debian/Ubuntu)" >&2
  exit 1
fi

# Empty directories aren't tracked by git, so a fresh checkout won't have
# $OUT's parent dir — same lesson as bootstrap-build.sh's `mkdir -p bin`.
mkdir -p "$(dirname "$OUT")"

# Same false-target-naming trap bootstrap-build.sh guards against: remove
# any stale output (and a stray .exe) before building, and hard-fail if the
# compiler didn't produce what was asked for.
rm -f "$OUT" "${OUT}.exe"
./bin/luma src/main.lx -l "${SRC_FILES[@]}" -O2 --no-sanitize -t "$TARGET" -name "$OUT"

FINAL="$OUT"
[ -f "${OUT}.exe" ] && FINAL="${OUT}.exe"
if [ ! -f "$FINAL" ]; then
  echo "error: cross-build for $TARGET did not produce '$OUT' (or '${OUT}.exe')" >&2
  exit 1
fi
echo "==> built $FINAL for $TARGET"
