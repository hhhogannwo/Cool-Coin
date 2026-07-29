#!/usr/bin/env bash

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd)"

DATA="$ROOT/data"
CONF="$DATA/coolcoin.conf"
BIN="$ROOT/bin"
CONFIG="$ROOT/config"
LOGS="$ROOT/logs"
RUNTIME="$ROOT/runtime"
SCRIPTS="$ROOT/scripts"
LIB="$ROOT/lib"

mkdir -p "$DATA" "$DATA/wallets" "$CONFIG" "$LOGS" "$RUNTIME"

export LD_LIBRARY_PATH="$LIB${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

CLI="$BIN/coolcoin-cli"
DAEMON="$BIN/coolcoind"
CKPOOL="$BIN/ckpool"
MINER="$BIN/minerd"

cli() {
    "$CLI" \
        -datadir="$DATA" \
        -conf="$CONF" \
        "$@"
}

wallet_cli() {
    "$CLI" \
        -datadir="$DATA" \
        -conf="$CONF" \
        -rpcwallet=coolwallet \
        "$@"
}

wait_for_rpc() {
    local attempt

    for attempt in $(seq 1 90); do
        if cli getblockchaininfo >/dev/null 2>&1; then
            return 0
        fi

        sleep 1
    done

    echo "ERROR: Cool Coin RPC did not become ready."
    return 1
}

ensure_wallet() {
    local loaded

    loaded="$(cli listwallets 2>/dev/null || true)"

    if grep -q '"coolwallet"' <<<"$loaded"; then
        return 0
    fi

    if cli loadwallet coolwallet >/dev/null 2>&1; then
        return 0
    fi

    if cli createwallet coolwallet >/dev/null 2>&1; then
        return 0
    fi

    loaded="$(cli listwallets 2>/dev/null || true)"

    if ! grep -q '"coolwallet"' <<<"$loaded"; then
        echo "ERROR: Could not create or load coolwallet."
        return 1
    fi
}
