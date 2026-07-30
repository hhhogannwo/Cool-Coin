#!/usr/bin/env bash
set -euo pipefail

SOURCE_DIR="."
ARCHIVE="${1:-$HOME/Downloads/CoolWallet-v2-all-buttons-fixed.tar.gz}"

if [ ! -f "$ARCHIVE" ]; then
  echo "ERROR: Archive not found: $ARCHIVE"
  echo "Put CoolWallet-v2-all-buttons-fixed.tar.gz in Downloads."
  exit 1
fi

ROOT="$(mktemp -d)"
trap 'rm -rf "$ROOT"' EXIT

tar -xzf "$ARCHIVE" -C "$ROOT"
FIXED="$ROOT/CoolWallet-v2"

if [ ! -f "$FIXED/src/mainwindow.cpp" ]; then
  echo "ERROR: Fixed source was not found in the archive."
  exit 1
fi

if [ -d "$SOURCE_DIR" ]; then
  BACKUP="${SOURCE_DIR}-before-all-buttons-$(date +%Y%m%d-%H%M%S)"
  cp -a "$SOURCE_DIR" "$BACKUP"
  echo "Backup: $BACKUP"
fi

mkdir -p "$SOURCE_DIR"
cp -a "$FIXED/." "$SOURCE_DIR/"
chmod +x "$SOURCE_DIR/scripts/start-mining.sh" 2>/dev/null || true

cd "$SOURCE_DIR"
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel "$(nproc)"

EXE="$(find build -type f -name CoolWallet -executable | head -n1)"
if [ -z "$EXE" ]; then
  echo "ERROR: Build completed but CoolWallet was not found."
  exit 1
fi

pkill -x CoolWallet 2>/dev/null || true
sleep 2
nohup "$EXE" > "$SOURCE_DIR/coolwallet-all-buttons.log" 2>&1 &

echo "CoolWallet started: $EXE"
echo "Log: $SOURCE_DIR/coolwallet-all-buttons.log"
