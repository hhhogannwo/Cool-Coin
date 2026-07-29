#!/usr/bin/env bash
set -Eeuo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
DATA="$ROOT/data"
CONF="$DATA/coolcoin.conf"
CLI="$ROOT/bin/coolcoin-cli"

export LD_LIBRARY_PATH="$ROOT/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

mkdir -p \
    "$DATA" \
    "$DATA/wallets" \
    "$ROOT/runtime" \
    "$ROOT/logs" \
    "$ROOT/run" \
    "$ROOT/config"

touch "$CONF"

ensure_setting() {
    local key="$1"
    local value="$2"

    if grep -qE "^[[:space:]]*${key}=" "$CONF"; then
        sed -i -E "s|^[[:space:]]*${key}=.*|${key}=${value}|" "$CONF"
    else
        printf '%s=%s\n' "$key" "$value" >> "$CONF"
    fi
}

ensure_setting server 1
ensure_setting daemon 1
ensure_setting listen 1
ensure_setting port 6464
ensure_setting rpcbind 127.0.0.1
ensure_setting rpcallowip 127.0.0.1
ensure_setting rpcport 6463
ensure_setting txindex 1
ensure_setting fallbackfee 0.00001000
ensure_setting wallet coolwallet
ensure_setting listenonion 0
ensure_setting networkactive 1

# Create unique RPC credentials on each new installation.
if ! grep -qE '^rpcuser=' "$CONF"; then
    RPCUSER="coolrpc_$(od -An -N6 -tx1 /dev/urandom | tr -d ' \n')"
    printf 'rpcuser=%s\n' "$RPCUSER" >> "$CONF"
fi

if ! grep -qE '^rpcpassword=' "$CONF"; then
    RPCPASS="$(od -An -N24 -tx1 /dev/urandom | tr -d ' \n')"
    printf 'rpcpassword=%s\n' "$RPCPASS" >> "$CONF"
fi

chmod 600 "$CONF"

# Remove an unusable copied txindex. The node rebuilds it automatically.
if grep -q "txindex: best block of the index not found" "$DATA/debug.log" 2>/dev/null; then
    rm -rf "$DATA/indexes/txindex"
fi

exit 0
