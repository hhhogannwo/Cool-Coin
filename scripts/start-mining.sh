#!/usr/bin/env bash
set -Eeuo pipefail

source "$(dirname "$0")/common.sh"

"$SCRIPTS/start-node.sh"

ensure_wallet

RPC_USER="$(sed -n 's/^rpcuser=//p' "$CONF" | tail -1)"
RPC_PASS="$(sed -n 's/^rpcpassword=//p' "$CONF" | tail -1)"

if [[ -z "$RPC_USER" || -z "$RPC_PASS" ]]; then
    echo "ERROR: RPC credentials are missing from:"
    echo "$CONF"
    exit 1
fi

MINING_ADDRESS="$(wallet_cli getnewaddress "CoolWallet Mining" bech32 2>/dev/null || true)"

if [[ -z "$MINING_ADDRESS" ]]; then
    MINING_ADDRESS="$(wallet_cli getnewaddress 2>/dev/null)"
fi

if [[ -z "$MINING_ADDRESS" ]]; then
    echo "ERROR: Could not create a mining address."
    exit 1
fi

python3 - "$CONFIG/ckpool.conf" "$RPC_USER" "$RPC_PASS" \
          "$MINING_ADDRESS" "$LOGS/ckpool" <<'PY'
import json
from pathlib import Path
import sys

path = Path(sys.argv[1])
rpc_user = sys.argv[2]
rpc_pass = sys.argv[3]
address = sys.argv[4]
logdir = sys.argv[5]

config = {
    "btcd": [
        {
            "url": "127.0.0.1:6463",
            "auth": rpc_user,
            "pass": rpc_pass,
            "notify": True
        }
    ],
    "btcaddress": address,
    "serverurl": [
        "127.0.0.1:3333"
    ],
    "mindiff": 1,
    "startdiff": 1,
    "logdir": logdir,
    "maxclients": 512
}

path.parent.mkdir(parents=True, exist_ok=True)
path.write_text(json.dumps(config, indent=2) + "\n")
path.chmod(0o600)
PY

mkdir -p "$LOGS/ckpool"

pkill -x minerd 2>/dev/null || true
pkill -x ckpool 2>/dev/null || true
sleep 2

rm -f \
    "$RUNTIME/ckpool.pid" \
    "$RUNTIME/minerd.pid"

# Clear results from previous mining sessions.
rm -f "$LOGS/ckpool-console.log" "$LOGS/minerd.log"
mkdir -p "$LOGS/ckpool" "$RUNTIME"

echo "Starting CKPool..."

nohup "$CKPOOL" \
    -c "$CONFIG/ckpool.conf" \
    >>"$LOGS/ckpool-console.log" 2>&1 &

echo $! > "$RUNTIME/ckpool.pid"

for attempt in $(seq 1 30); do
    if ss -ltn 2>/dev/null | grep -q '127.0.0.1:3333'; then
        break
    fi

    sleep 1
done

if ! ss -ltn 2>/dev/null | grep -q '127.0.0.1:3333'; then
    echo "ERROR: CKPool did not open port 3333."
    tail -50 "$LOGS/ckpool-console.log" 2>/dev/null || true
    exit 1
fi

sleep 2

if grep -qE '401 Unauthorized|No bitcoinds active' \
    "$LOGS/ckpool-console.log" 2>/dev/null; then
    echo "ERROR: CKPool RPC authentication failed."
    tail -30 "$LOGS/ckpool-console.log"
    exit 1
fi

THREADS="${COOLWALLET_MINER_THREADS:-$(nproc)}"


echo "Waiting for CKPool Stratum port..."

STRATUM_READY=0
for attempt in $(seq 1 30); do
    if ss -ltn 2>/dev/null | grep -qE '127\.0\.0\.1:3333|0\.0\.0\.0:3333'; then
        STRATUM_READY=1
        break
    fi

    if ! pgrep -x ckpool >/dev/null 2>&1; then
        echo "ERROR: CKPool exited before opening port 3333."
        tail -50 "$LOGS/ckpool-console.log" 2>/dev/null || true
        exit 1
    fi

    sleep 1
done

if [ "$STRATUM_READY" -ne 1 ]; then
    echo "ERROR: CKPool did not open Stratum port 3333."
    tail -50 "$LOGS/ckpool-console.log" 2>/dev/null || true
    exit 1
fi

echo "CKPool Stratum is ready."
sleep 2

echo "Starting CPU miner with $THREADS threads..."

nohup "$MINER" \
    -a sha256d \
    -o stratum+tcp://127.0.0.1:3333 \
    -u "$MINING_ADDRESS" \
    -p x \
    -t "$THREADS" \
    >>"$LOGS/minerd.log" 2>&1 &

echo $! > "$RUNTIME/minerd.pid"

sleep 5

if ! pgrep -x ckpool >/dev/null; then
    echo "ERROR: CKPool stopped."
    tail -50 "$LOGS/ckpool-console.log" 2>/dev/null || true
    exit 1
fi

if ! pgrep -x minerd >/dev/null; then
    echo "ERROR: CPU miner stopped."
    tail -50 "$LOGS/minerd.log" 2>/dev/null || true
    exit 1
fi

echo
echo "MINING IS ACTIVE"
echo "Mining address: $MINING_ADDRESS"
echo "Stratum: 127.0.0.1:3333"
echo "Threads: $THREADS"
