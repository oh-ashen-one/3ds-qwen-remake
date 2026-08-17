#!/bin/bash
# supervisor.sh — phase-gated loop driver. Runs on the STUDIO in tmux
# `ralph-3ds` under caffeinate. Parks at checkpoints until a human bumps PHASE.
set -uo pipefail
cd "$(dirname "$0")/../.."

while true; do
  bash scripts/ralph/ralph.sh
  CODE=$?
  if [ $CODE -eq 42 ]; then
    P=$(cat PHASE)
    scripts/ops/notify.sh "🛑 CHECKPOINT: phase $P stories all pass. Loop parked — manager must enrich next-phase stories and bump PHASE to resume." || true
    while [ "$(cat PHASE)" = "$P" ]; do sleep 60; done
    scripts/ops/notify.sh "▶️ resuming at phase $(cat PHASE)" || true
  elif [ $CODE -eq 0 ]; then
    scripts/ops/notify.sh "🏁 supervisor: all stories complete. Exiting." || true
    exit 0
  else
    scripts/ops/notify.sh "⚠️ ralph.sh exited $CODE — pausing 2 min then retrying. Log tail: $(tail -c 300 ralph.log 2>/dev/null)" || true
    sleep 120
  fi
done
