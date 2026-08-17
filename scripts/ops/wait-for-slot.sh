#!/bin/bash
# wait-for-slot.sh — queue the 3DS loop behind the other Qwen ralph loops.
# Runs on the STUDIO in tmux `ralph-3ds-wait`.
#
# Rule (from the human): at most THREE loops generate at once. The peers are
# fifa-ralph, ralph-showcase, and driftwood-ralph (about to start — reserved
# even while idle until it has been seen generating). We launch only when a
# peer ENDS, leaving <=2 active, so ours makes three.
#
# A peer counts as ended when its lms instance is gone, or has been IDLE for
# IDLE_POLLS consecutive polls (verify phases idle for minutes, not 30).
set -uo pipefail
export PATH="$HOME/.lmstudio/bin:/opt/homebrew/bin:/usr/bin:/bin"
PEERS=(fifa-ralph ralph-showcase driftwood-ralph)
IDLE_POLLS=6          # 6 x 300s = 30 min sustained idle
POLL=300
STATE_DIR="$HOME/.ralph3ds-wait"
mkdir -p "$STATE_DIR"

while true; do
  PS=$(lms ps 2>/dev/null || true)
  ACTIVE=0
  SUMMARY=""
  for P in "${PEERS[@]}"; do
    LINE=$(echo "$PS" | awk -v p="$P" '$1==p {print $3}')
    STREAK_F="$STATE_DIR/$P.idle"
    SEEN_F="$STATE_DIR/$P.seen-generating"
    if [ -z "$LINE" ]; then
      SUMMARY="$SUMMARY $P=gone"
      continue                       # unloaded => ended
    fi
    if [ "$LINE" = "IDLE" ]; then
      STREAK=$(( $(cat "$STREAK_F" 2>/dev/null || echo 0) + 1 ))
      echo "$STREAK" > "$STREAK_F"
    else
      echo 0 > "$STREAK_F"
      touch "$SEEN_F"
      STREAK=0
    fi
    # driftwood is reserved while merely idle-before-first-run
    if [ "$P" = "driftwood-ralph" ] && [ ! -f "$SEEN_F" ]; then
      ACTIVE=$((ACTIVE + 1)); SUMMARY="$SUMMARY $P=reserved"
    elif [ "$STREAK" -ge "$IDLE_POLLS" ]; then
      SUMMARY="$SUMMARY $P=ended(idle x$STREAK)"
    else
      ACTIVE=$((ACTIVE + 1)); SUMMARY="$SUMMARY $P=active($LINE,idle$STREAK)"
    fi
  done
  echo "$(date '+%H:%M') active=$ACTIVE:$SUMMARY"
  if [ "$ACTIVE" -le 2 ]; then
    "$(dirname "$0")/notify.sh" "⏳→🚀 slot free ($ACTIVE peers active:$SUMMARY) — launching ralph-3ds" || true
    exec "$(dirname "$0")/launch.sh"
  fi
  sleep "$POLL"
done
