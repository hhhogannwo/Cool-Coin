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
CKPOOL_PORT=3334

mkdir -p "$DATA" "$CONFIG" "$ROOT/logs" "$ROOT/runtime"

for executable in "$CLI" "$CKPOOL" "$MINER"; do
    if [ ! -x "$executable" ]; then
        echo "Missing executable: $executable" >&2
        exit 1
    fi
done

if [ ! -f "$CONF" ]; then
    echo "Missing configuration: $CONF" >&2
    exit 1
fi

read_conf_value() {
    local key="$1"
    awk -F= -v key="$key" '
        /^[[:space:]]*#/ { next }
        {
            name=$1
            gsub(/^[[:space:]]+|[[:space:]]+$/, "", name)
            if (name == key) {
                value=substr($0, index($0, "=") + 1)
                gsub(/^[[:space:]]+|[[:space:]]+$/, "", value)
                result=value
            }
        }
        END { print result }
    ' "$CONF"
}

json_escape() {
    printf '%s' "$1" | sed 's/\\/\\\\/g; s/"/\\"/g'
}

if [ ! -f "$CKCONF" ]; then
    RPC_USER="$(read_conf_value rpcuser)"
    RPC_PASSWORD="$(read_conf_value rpcpassword)"
    RPC_HOST="$(read_conf_value rpcconnect)"
    RPC_PORT="$(read_conf_value rpcport)"

    RPC_HOST="${RPC_HOST:-127.0.0.1}"
    RPC_PORT="${RPC_PORT:-6463}"

    if [ -z "$RPC_USER" ] || [ -z "$RPC_PASSWORD" ]; then
        echo "coolcoin.conf must contain rpcuser and rpcpassword before local mining can start." >&2
        exit 1
    fi

    MINING_ADDRESS="${COOLCOIN_MINING_ADDRESS:-}"
    if [ -z "$MINING_ADDRESS" ]; then
        MINING_ADDRESS="$("$CLI" -datadir="$DATA" -conf="$CONF" -rpcwallet=coolwallet getnewaddress "ckpool" "bech32" 2>/dev/null || true)"
    fi

    if [ -z "$MINING_ADDRESS" ]; then
        echo "Could not obtain a mining address from wallet 'coolwallet'." >&2
        echo "Load the wallet, or set COOLCOIN_MINING_ADDRESS to one of your Cool Coin addresses." >&2
        exit 1
    fi

    CKCONF_TMP="$CKCONF.tmp"
    cat > "$CKCONF_TMP" <<EOF
{
  "btcd": [
    {
      "url": "$(json_escape "$RPC_HOST"):$RPC_PORT",
      "auth": "$(json_escape "$RPC_USER")",
      "pass": "$(json_escape "$RPC_PASSWORD")",
      "notify": true
    }
  ],
  "btcaddress": "$(json_escape "$MINING_ADDRESS")",
  "btcsig": "/Cool Coin/",
  "serverurl": [
    "127.0.0.1:$CKPOOL_PORT"
  ],
  "mindiff": 1,
  "startdiff": 1,
  "maxclients": 64
}
EOF
    chmod 600 "$CKCONF_TMP"
    mv "$CKCONF_TMP" "$CKCONF"
    echo "Created local CKPool configuration: $CKCONF"
fi

pkill -f "$CKPOOL" 2>/dev/null || true
pkill -f "$MINER" 2>/dev/null || true

nohup "$CKPOOL" -c "$CKCONF" \
    > "$ROOT/logs/ckpool.log" 2>&1 &
CKPOOL_PID=$!
echo "$CKPOOL_PID" > "$ROOT/runtime/ckpool.pid"

sleep 3

if ! kill -0 "$CKPOOL_PID" 2>/dev/null; then
    echo "CKPool failed to start. Check $ROOT/logs/ckpool.log" >&2
    exit 1
fi

nohup "$MINER" \
    -a sha256d \
    -o "stratum+tcp://127.0.0.1:$CKPOOL_PORT" \
    -u coolwallet \
    -p x \
    > "$ROOT/logs/minerd.log" 2>&1 &
MINER_PID=$!
echo "$MINER_PID" > "$ROOT/runtime/minerd.pid"

sleep 2

if ! kill -0 "$MINER_PID" 2>/dev/null; then
    echo "Miner failed to start. Check $ROOT/logs/minerd.log" >&2
    kill "$CKPOOL_PID" 2>/dev/null || true
    exit 1
fi

echo "Cool Coin mining started on local Stratum port $CKPOOL_PORT."
