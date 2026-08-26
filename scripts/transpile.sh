#!/usr/bin/env bash
# Transpiles the luma compiler ITSELF to C for another OS, without trying
# to compile/link it — using a native bin/luma (already built by
# bootstrap-build.sh) with -t <target> -c/--no-compile.
#
# This is the supported path for a target with no real cross-compiler
# available on this host (macOS, from Linux — only osxcross exists, and it
# needs Apple's SDK extracted from a real Xcode install). Hand the emitted
# .c to that platform's own `cc` to finish the build. See
# .github/workflows/release.yml's build-macos job for exactly that: this
# script runs on a Linux runner, then a macos-latest runner compiles the
# result with its own preinstalled clang.
#
# For Windows, prefer scripts/cross-build.sh instead — mingw-w64 is a real
# cross-compiler, so that path produces a linked binary directly, no
# second native-compile step needed.
#
# Usage:
#   scripts/transpile.sh <target-os> <output.c>
#
# Requires: bin/luma already built.
set -euo pipefail

cd "$(dirname "$0")/.."
source "scripts/lib-src-files.sh"

TARGET="${1:?usage: transpile.sh <target-os> <output.c>}"
OUT="${2:?usage: transpile.sh <target-os> <output.c>}"

if [ ! -x "bin/luma" ]; then
  echo "error: bin/luma not found — run scripts/bootstrap-build.sh first" >&2
  exit 1
fi

# Empty directories aren't tracked by git, so a fresh checkout won't have
# $OUT's parent dir.
mkdir -p "$(dirname "$OUT")"

# codegen_generate always writes to output/<basename(-name)>.c — -name here
# is just that basename, not a real binary name (nothing gets linked).
NAME="$(basename "$OUT" .c)"
rm -rf output
./bin/luma src/main.lx -l "${SRC_FILES[@]}" -O2 --no-sanitize -t "$TARGET" -c -name "$NAME"

if [ ! -f "output/${NAME}.c" ]; then
  echo "error: transpile for $TARGET did not produce 'output/${NAME}.c'" >&2
  exit 1
fi
mv "output/${NAME}.c" "$OUT"
echo "==> transpiled $OUT for $TARGET (not compiled — needs a native $TARGET compiler)"
