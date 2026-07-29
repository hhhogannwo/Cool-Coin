#!/usr/bin/env bash
set -Eeuo pipefail

source "$(dirname "$0")/common.sh"

pkill -x minerd 2>/dev/null || true
pkill -x ckpool 2>/dev/null || true

rm -f \
    "$RUNTIME/minerd.pid" \
    "$RUNTIME/ckpool.pid"

echo "Cool Coin mining stopped."
