#!/bin/bash
# supervisor.sh — phase-gated loop driver. Runs on the STUDIO in tmux
# `ralph-3ds` under caffeinate. Parks at checkpoints until a human bumps PHASE.
set -uo pipefail
cd "$(dirname "$0")/../.."

# HARD RULE: release our ralph slot when the loop stops for any reason.
trap 'bash ~/ralph-slots/release.sh 3ds-remake 2>/dev/null || true' EXIT

while true; do
  bash scripts/ralph/ralph.sh
  CODE=$?
  if [ $CODE -eq 42 ]; then
    P=$(cat PHASE)
    # Parked = not generating: give the slot back so the queue moves (HARD RULE).
    bash ~/ralph-slots/release.sh 3ds-remake 2>/dev/null || true
    scripts/ops/notify.sh "🛑 CHECKPOINT: phase $P stories all pass. Loop parked, slot released — manager must enrich next-phase stories and bump PHASE to resume." || true
    while [ "$(cat PHASE)" = "$P" ]; do sleep 60; done
    scripts/ops/notify.sh "▶️ PHASE bumped to $(cat PHASE) — re-queueing for a slot" || true
    grep -qx "3ds-remake" ~/ralph-slots/QUEUE 2>/dev/null || echo "3ds-remake" >> ~/ralph-slots/QUEUE
    bash ~/ralph-slots/claim.sh "3ds-remake" "3ds-qwen-remake/scripts/ops/"
    scripts/ops/notify.sh "▶️ slot re-claimed — resuming at phase $(cat PHASE)" || true
  elif [ $CODE -eq 0 ]; then
    scripts/ops/notify.sh "🏁 supervisor: all stories complete. Exiting." || true
    exit 0
  else
    scripts/ops/notify.sh "⚠️ ralph.sh exited $CODE — pausing 2 min then retrying. Log tail: $(tail -c 300 ralph.log 2>/dev/null)" || true
    sleep 120
  fi
done
