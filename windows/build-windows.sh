#!/usr/bin/env bash
set -Eeuo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST="$ROOT/dist"
STAGE="$DIST/CoolWallet-Windows-x86_64"
GUI_SRC="$ROOT/CoolWallet-v2"
GUI_BUILD="$GUI_SRC/build-windows"

rm -rf "$DIST" "$GUI_BUILD"
mkdir -p "$STAGE/bin" "$STAGE/data" "$STAGE/platforms"

echo "== Building Cool Coin daemon and CLI =="

cd "$ROOT"

if [[ -x ./autogen.sh ]]; then
  ./autogen.sh
fi

if [[ ! -x ./configure ]]; then
  echo "ERROR: ./configure is missing."
  exit 1
fi

./configure \
  --disable-tests \
  --disable-bench \
  --without-miniupnpc \
  --without-natpmp

make -j"$(nproc)"

DAEMON="$(find "$ROOT" -type f -name 'coolcoind.exe' | head -n1 || true)"
CLI="$(find "$ROOT" -type f -name 'coolcoin-cli.exe' | head -n1 || true)"

[[ -f "$DAEMON" ]] || { echo "ERROR: coolcoind.exe not found"; exit 1; }
[[ -f "$CLI" ]] || { echo "ERROR: coolcoin-cli.exe not found"; exit 1; }

cp -f "$DAEMON" "$STAGE/bin/coolcoind.exe"
cp -f "$CLI" "$STAGE/bin/coolcoin-cli.exe"

echo "== Building CoolWallet GUI =="

[[ -f "$GUI_SRC/CMakeLists.txt" ]] || {
  echo "ERROR: Expected GUI source at $GUI_SRC"
  exit 1
}

cmake -S "$GUI_SRC" -B "$GUI_BUILD" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/ucrt64

cmake --build "$GUI_BUILD" --parallel

GUI="$(find "$GUI_BUILD" -type f -name 'CoolWallet.exe' | head -n1 || true)"
[[ -f "$GUI" ]] || { echo "ERROR: CoolWallet.exe not found"; exit 1; }

cp -f "$GUI" "$STAGE/CoolWallet.exe"

echo "== Deploying Qt runtime =="

WDEPLOYQT="/ucrt64/bin/windeployqt.exe"
[[ -x "$WDEPLOYQT" ]] || { echo "ERROR: windeployqt.exe not found"; exit 1; }

"$WDEPLOYQT" \
  --release \
  --no-translations \
  --compiler-runtime \
  "$STAGE/CoolWallet.exe"

cp -f "$ROOT/windows/START-COOLWALLET.bat" "$STAGE/START-COOLWALLET.bat"
cp -f "$ROOT/windows/STOP-COOLWALLET.bat" "$STAGE/STOP-COOLWALLET.bat"
cp -f "$ROOT/windows/README-WINDOWS.txt" "$STAGE/README-WINDOWS.txt"

cat > "$STAGE/data/coolcoin.conf" <<'EOF'
server=1
listen=1
daemon=0
txindex=1
discover=1
fallbackfee=0.00001000
rpcbind=127.0.0.1
rpcallowip=127.0.0.1
rpcport=6463
port=6464
addnode=10.0.0.26:6464
addnode=10.0.0.37:6464
addnode=10.0.0.106:6464
EOF

echo "== Validating Windows binaries =="

for f in \
  "$STAGE/CoolWallet.exe" \
  "$STAGE/bin/coolcoind.exe" \
  "$STAGE/bin/coolcoin-cli.exe"
do
  file "$f"
  if file "$f" | grep -qi ELF; then
    echo "ERROR: $f is a Linux binary."
    exit 1
  fi
done

echo "== Packaging =="

cd "$DIST"
zip -r -9 CoolWallet-Windows-x86_64.zip CoolWallet-Windows-x86_64
sha256sum CoolWallet-Windows-x86_64.zip > SHA256SUMS-Windows.txt

echo
echo "Created:"
ls -lh CoolWallet-Windows-x86_64.zip SHA256SUMS-Windows.txt
