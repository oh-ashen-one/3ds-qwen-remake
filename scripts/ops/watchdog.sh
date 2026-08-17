#!/bin/bash
# watchdog.sh — LAPTOP cron (*/5). Vitals over ONE ssh call; dumb-fix once per
# new incident; alert via Studio-side notify.sh. Never touches fifa-ralph or
# ralph-showcase. Does nothing while the loop hasn't been started yet.
set -uo pipefail
export PATH="/opt/homebrew/bin:/usr/bin:/bin"
STATE="$HOME/.ralph3ds-watchdog-state"
SSH=(ssh -o ConnectTimeout=12 -o BatchMode=yes studio)

VITALS=$("${SSH[@]}" 'export PATH="$HOME/.lmstudio/bin:/opt/homebrew/bin:/usr/bin:/bin"
  B=$HOME/3ds-qwen-remake
  tmux has-session -t ralph-3ds 2>/dev/null && echo TMUX=up || echo TMUX=down
  lms ps 2>/dev/null | grep -q ralph-3ds && echo MODEL=loaded || echo MODEL=missing
  curl -sf --max-time 5 http://127.0.0.1:1234/v1/models >/dev/null && echo API=up || echo API=down
  echo PRD_AGE=$(( $(date +%s) - $(stat -f %m "$B/progress.txt" 2>/dev/null || echo 0) ))
  tail -40 "$B/progress.txt" 2>/dev/null | grep -c "verify_ok: False" | sed "s/^/RECENT_FAILS=/"
') || { echo "watchdog: studio unreachable"; exit 0; }

TMUX=$(echo "$VITALS" | grep -o 'TMUX=[a-z]*' | cut -d= -f2)
MODEL=$(echo "$VITALS" | grep -o 'MODEL=[a-z]*' | cut -d= -f2)
API=$(echo "$VITALS" | grep -o 'API=[a-z]*' | cut -d= -f2)
PRD_AGE=$(echo "$VITALS" | grep -o 'PRD_AGE=[0-9]*' | cut -d= -f2)

# Loop not started yet (no tmux ever) and no incident state: stay silent.
if [ "$TMUX" = "down" ] && [ ! -f "$STATE.started" ]; then exit 0; fi
[ "$TMUX" = "up" ] && touch "$STATE.started"

INCIDENT=""
[ "$TMUX" = "down" ] && INCIDENT="tmux-down"
[ "$MODEL" = "missing" ] && INCIDENT="$INCIDENT model-missing"
[ "$API" = "down" ] && INCIDENT="$INCIDENT api-down"
# Dead-man: 45 min without a prd/progress touch while loop is up (> max iter 30m).
[ "$TMUX" = "up" ] && [ "${PRD_AGE:-0}" -gt 2700 ] && INCIDENT="$INCIDENT stale-${PRD_AGE}s"

LAST=$(cat "$STATE" 2>/dev/null || true)
if [ -z "$INCIDENT" ]; then rm -f "$STATE"; exit 0; fi
[ "$INCIDENT" = "$LAST" ] && exit 0   # same incident: already handled once
echo "$INCIDENT" > "$STATE"

FIXES=""
if [ "$MODEL" = "missing" ]; then
  "${SSH[@]}" 'export PATH="$HOME/.lmstudio/bin:$PATH"; lms load qwen3.8-27b@8bit --identifier ralph-3ds --context-length 131072 -y' \
    && FIXES="$FIXES reloaded-model" || FIXES="$FIXES model-reload-FAILED"
fi
if [ "$TMUX" = "down" ] && [ -f "$STATE.started" ]; then
  "${SSH[@]}" 'cd ~/3ds-qwen-remake && tmux new -d -s ralph-3ds "caffeinate -i scripts/ops/supervisor.sh >> ralph.log 2>&1"' \
    && FIXES="$FIXES restarted-tmux" || FIXES="$FIXES tmux-restart-FAILED"
fi

"${SSH[@]}" "cd ~/3ds-qwen-remake && scripts/ops/notify.sh '🚨 watchdog: $INCIDENT | fixes:${FIXES:- none} | $(echo "$VITALS" | tr "\n" " ")'" || true
