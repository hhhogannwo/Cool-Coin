#!/usr/bin/env bash
set -Eeuo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SOURCE="$ROOT/macos/CoolWallet-v2"
BUILD="$ROOT/build-macos"
DIST="$ROOT/dist"
PACKAGE="$DIST/CoolWallet-macOS-Intel"

rm -rf "$BUILD" "$PACKAGE"
mkdir -p "$BUILD" "$PACKAGE" "$DIST"

QT_PREFIX="$(brew --prefix qt@5)"

cmake \
  -S "$SOURCE" \
  -B "$BUILD" \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$QT_PREFIX"

cmake --build "$BUILD" --parallel

APP="$BUILD/CoolWallet.app"

if [[ ! -d "$APP" ]]; then
    echo "ERROR: CoolWallet.app was not created."
    find "$BUILD" -maxdepth 3 -print
    exit 1
fi

"$QT_PREFIX/bin/macdeployqt" "$APP" -verbose=2

cp -R "$APP" "$PACKAGE/"

cat > "$PACKAGE/README.txt" <<'README'
CoolWallet for macOS Intel
Version 27.1.0

This initial macOS package contains the native CoolWallet graphical
application.

The Cool Coin daemon, command-line client, CKPool, and CPU miner are
not yet included in this first macOS test build.

Because this build is not yet signed or notarized, macOS may require
the user to approve it through Privacy & Security.
README

(
    cd "$DIST"
    ditto \
      -c \
      -k \
      --sequesterRsrc \
      --keepParent \
      "CoolWallet-macOS-Intel" \
      "CoolWallet-macOS-Intel.zip"
)

shasum -a 256 \
  "$DIST/CoolWallet-macOS-Intel.zip" \
  > "$DIST/SHA256SUMS-macOS-Intel.txt"

echo "===== MACOS PACKAGE ====="
find "$PACKAGE" -maxdepth 5 -print

echo
echo "===== CHECKSUM ====="
cat "$DIST/SHA256SUMS-macOS-Intel.txt"
