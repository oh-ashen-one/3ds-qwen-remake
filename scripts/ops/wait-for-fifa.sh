#!/bin/bash
# wait-for-fifa.sh — queue the 3DS loop behind fifa-ralph. Runs on the STUDIO
# in tmux `ralph-3ds-wait`. Chains on STATE, not process-exit (failure #30):
# fifa is done only when its prd has zero unpassed stories, or its tmux
# session has been gone for 3 consecutive checks (manager killed it).
set -uo pipefail
export PATH="/opt/homebrew/bin:/usr/bin:/bin"
FIFA_PRD="$HOME/fifa-2026/scripts/ralph/prd.json"
GONE=0

while true; do
  LEFT=$(jq '[.userStories[]|select(.passes|not)]|length' "$FIFA_PRD" 2>/dev/null || echo 99)
  if [ "$LEFT" = "0" ]; then
    echo "fifa prd complete"
    break
  fi
  if tmux has-session -t fifa 2>/dev/null || tmux list-sessions 2>/dev/null | grep -qi fifa; then
    GONE=0
  else
    GONE=$((GONE + 1))
    [ $GONE -ge 3 ] && { echo "fifa tmux gone x3"; break; }
  fi
  sleep 300
done

"$(dirname "$0")/notify.sh" "⏳→🚀 fifa-ralph finished (stories left: ${LEFT:-?}) — launching ralph-3ds" || true
exec "$(dirname "$0")/launch.sh"
