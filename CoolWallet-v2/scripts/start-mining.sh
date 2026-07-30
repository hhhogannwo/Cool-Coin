#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DATA="$ROOT/data"
BIN="$ROOT/bin"
CONFIG="$ROOT/config"

CLI="$BIN/coolcoin-cli"
CKPOOL="$BIN/ckpool"
MINER="$BIN/minerd"
CONF="$DATA/coolcoin.conf"
CKCONF="$CONFIG/ckpool.conf"

mkdir -p "$DATA" "$CONFIG" "$ROOT/logs" "$ROOT/runtime"

if [ ! -x "$CLI" ]; then
    echo "Missing executable: $CLI"
    exit 1
fi

if [ ! -x "$CKPOOL" ]; then
    echo "Missing executable: $CKPOOL"
    exit 1
fi

if [ ! -x "$MINER" ]; then
    echo "Missing executable: $MINER"
    exit 1
fi

if [ ! -f "$CONF" ]; then
    echo "Missing configuration: $CONF"
    exit 1
fi

if [ ! -f "$CKCONF" ]; then
    echo "Missing configuration: $CKCONF"
    exit 1
fi

pkill -f "$CKPOOL" 2>/dev/null || true
pkill -f "$MINER" 2>/dev/null || true

nohup "$CKPOOL" -c "$CKCONF" \
    > "$ROOT/logs/ckpool.log" 2>&1 &

echo $! > "$ROOT/runtime/ckpool.pid"

sleep 3

nohup "$MINER" \
    -a sha256d \
    -o stratum+tcp://127.0.0.1:3333 \
    -u coolwallet \
    -p x \
    > "$ROOT/logs/minerd.log" 2>&1 &

echo $! > "$ROOT/runtime/minerd.pid"

echo "Cool Coin mining started."
