#!/bin/bash

LUMA_BIN="./build/luma"
TEST_DIR="tests/errors"
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'
PASS=0
FAIL=0
CRASH=0
TIMING=0

if [ "$1" = "--timing" ]; then
  TIMING=1
  shift
fi

if [ $# -gt 0 ]; then
  TESTS=()
  for name in "$@"; do
    TESTS+=("$TEST_DIR/$name")
  done
else
  TESTS=("$TEST_DIR"/*.lx)
fi

if [ ! -f "$LUMA_BIN" ]; then
  echo "Building luma..."
  meson compile -C build/ 2>&1 || { echo "Build failed"; exit 1; }
fi

total=${#TESTS[@]}
count=0

for test_file in "${TESTS[@]}"; do
  if [ ! -f "$test_file" ]; then
    echo -e "${YELLOW}SKIP${NC}: $test_file (not found)"
    continue
  fi
  count=$((count + 1))
  name=$(basename "$test_file")

  start=${EPOCHREALTIME:-0}
  ./build/luma "$test_file" > /dev/null 2>&1
  rc=$?
  if [ "$TIMING" = "1" ] && [ "$start" != "0" ]; then
    elapsed=$(echo "$(date +%s%N) - $start" | bc 2>/dev/null || echo 0)
  fi

  if [ $rc -eq 139 ]; then
    echo -e "[${RED}CRASH${NC}] $name"
    CRASH=$((CRASH + 1))
  elif [ $rc -ne 0 ]; then
    echo -e "[${GREEN}PASS${NC}] $name"
    PASS=$((PASS + 1))
  else
    echo -e "[${YELLOW}FAIL${NC}] $name (exit=0, expected error)"
    FAIL=$((FAIL + 1))
  fi
done

echo
echo "Results: ${PASS} pass, ${FAIL} fail, ${CRASH} crash (${count} total)"
