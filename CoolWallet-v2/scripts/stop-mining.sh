#!/usr/bin/env bash

pkill -f 'minerd.*127.0.0.1:3333' 2>/dev/null || true
pkill -f 'ckpool.*3333' 2>/dev/null || true

echo "Cool Coin mining stopped."
