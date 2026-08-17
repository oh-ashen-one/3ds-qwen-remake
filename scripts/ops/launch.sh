#!/bin/bash
# launch.sh — start the 3DS ralph loop on the STUDIO. Idempotent.
# Loads a DEDICATED model instance (never touches fifa-ralph / ralph-showcase)
# and starts tmux `ralph-3ds` running the supervisor under caffeinate.
set -uo pipefail
export PATH="$HOME/.lmstudio/bin:/opt/homebrew/bin:/usr/bin:/bin"
cd "$(dirname "$0")/../.."

if ! lms ps 2>/dev/null | grep -q '^ralph-3ds '; then
  lms load qwen3.8-27b@8bit --identifier ralph-3ds --context-length 131072 -y \
    || { scripts/ops/notify.sh "🚨 launch: model load failed"; exit 1; }
fi

# Sanity: endpoint + prefill workaround must answer before we loop.
python3 scripts/ralph/qwen_iteration.py --ping \
  || { scripts/ops/notify.sh "🚨 launch: --ping failed (model up but not answering)"; exit 1; }

if ! tmux has-session -t ralph-3ds 2>/dev/null; then
  tmux new -d -s ralph-3ds "caffeinate -i scripts/ops/supervisor.sh >> ralph.log 2>&1"
fi
scripts/ops/notify.sh "🚀 ralph-3ds launched: $(lms ps | grep ralph-3ds | head -1)"
echo "launched: tmux attach -t ralph-3ds"
