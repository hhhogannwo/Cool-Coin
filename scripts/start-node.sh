#!/usr/bin/env bash
set -Eeuo pipefail

source "$(dirname "$0")/common.sh"

if cli getblockchaininfo >/dev/null 2>&1; then
    echo "Cool Coin RPC is already ready."
else
    echo "Starting Cool Coin node..."

    "$DAEMON" \
        -datadir="$DATA" \
        -conf="$CONF" \
        -daemon \
        -server=1 \
        -listen=1
fi

echo "Waiting for Cool Coin RPC..."
wait_for_rpc

echo "Cool Coin RPC is ready."

ensure_wallet

echo "coolwallet is active."

for peer in \
    "10.0.0.26:6464" \
    "10.0.0.37:6464" \
    "10.0.0.106:6464"
do
    cli addnode "$peer" add >/dev/null 2>&1 || true
done

connections="$(cli getconnectioncount 2>/dev/null || echo 0)"
echo "Node connections: $connections"
