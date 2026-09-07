#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RUNTIME="$ROOT/runtime"

stop_process() {
    local name="$1"
    local pid_file="$2"

    if [ ! -f "$pid_file" ]; then
        return
    fi

    local pid
    pid="$(cat "$pid_file")"
    if [[ "$pid" =~ ^[0-9]+$ ]] && kill -0 "$pid" 2>/dev/null; then
        kill "$pid"
        echo "Stopped $name (PID $pid)."
    fi
    rm -f "$pid_file"
}

stop_process "miner" "$RUNTIME/minerd.pid"
stop_process "CKPool" "$RUNTIME/ckpool.pid"

# Clean up older portable-wallet processes that predate PID tracking.
pkill -f 'minerd.*127\.0\.0\.1:3333' 2>/dev/null || true
pkill -f 'ckpool.*ckpool\.conf' 2>/dev/null || true

echo "Cool Coin mining stopped."
