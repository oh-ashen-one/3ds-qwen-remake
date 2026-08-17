#!/bin/bash
# queued-start.sh — HARD RULE compliant start (AGENTS.md / ~/RALPH-RULES.md):
# max 2 Ralph loops generating on the Studio, mechanical slot claim.
# Runs on the STUDIO in tmux `ralph-3ds-wait`. Blocks in claim.sh until a
# slot frees AND "3ds-remake" is head of ~/ralph-slots/QUEUE, then launches.
#
# The slot's pgrep pattern matches this script, launch.sh, and the supervisor
# (all invoked by absolute path), so the reaper never reads our live claim as
# dead during the model-load window.
set -uo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
PATTERN="3ds-qwen-remake/scripts/ops/"

grep -qx "3ds-remake" ~/ralph-slots/QUEUE 2>/dev/null || echo "3ds-remake" >> ~/ralph-slots/QUEUE

"$DIR/notify.sh" "⏳ 3ds-remake queued for a ralph slot (order: $(grep -v '^#' ~/ralph-slots/QUEUE | tr '\n' ' '))" || true
bash ~/ralph-slots/claim.sh "3ds-remake" "$PATTERN"
"$DIR/notify.sh" "🎟️ slot claimed — launching ralph-3ds" || true
exec bash "$DIR/launch.sh"
