#!/usr/bin/env bash
set -euo pipefail

# Usage: ./kill_wordle.sh [port]
PORT="${1:-8080}"

# Match either spelling, anywhere on the command line
PATTERN='(wordle-server\.out|worlde-server\.out)'

die() { echo "$*" >&2; exit 1; }

# 1) Try to find by name
pids_by_name="$(pgrep -f "$PATTERN" || true)"

# 2) If not found by name, try by port (needs lsof)
pids_by_port="$(lsof -ti tcp:"$PORT" 2>/dev/null || true)"

# Merge unique PIDs
pids="$(printf "%s\n%s\n" "$pids_by_name" "$pids_by_port" | tr ' ' '\n' | sort -u | sed '/^$/d')"

if [[ -z "$pids" ]]; then
  die "No wordle server found by name or on port $PORT."
fi

echo "Found server PID(s): $pids"

# Try graceful shutdown first
kill -USR1 $pids 2>/dev/null || true
sleep 0.5

# If still alive, try TERM
if pgrep -f "$PATTERN" >/dev/null || lsof -ti tcp:"$PORT" >/dev/null 2>&1; then
  kill $pids 2>/dev/null || true
  sleep 0.5
fi

# If STILL alive, nuke it
if pgrep -f "$PATTERN" >/dev/null || lsof -ti tcp:"$PORT" >/dev/null 2>&1; then
  kill -9 $pids 2>/dev/null || true
fi

echo "Killed Wordle server (or it was already gone)."

