#!/bin/bash
# watchdog.sh — LAPTOP cron (*/5). Vitals over ONE ssh call; dumb-fix once per
# new incident; alert via Studio-side notify.sh. Never touches fifa-ralph or
# ralph-showcase. Does nothing while the loop hasn't been started yet.
# NOTE: tmux -t does PREFIX matching; always use =name for exact match
# (bare ralph-3ds matches ralph-3ds-wait — this bug caused a premature
# model load on 2026-08-16).
set -uo pipefail
export PATH="/opt/homebrew/bin:/usr/bin:/bin"
STATE="$HOME/.ralph3ds-watchdog-state"
SSH=(ssh -o ConnectTimeout=12 -o BatchMode=yes studio)

VITALS=$("${SSH[@]}" 'export PATH="$HOME/.lmstudio/bin:/opt/homebrew/bin:/usr/bin:/bin"
  B=$HOME/3ds-qwen-remake
  tmux has-session -t "=ralph-3ds" 2>/dev/null && echo LOOP=up || echo LOOP=down
  tmux has-session -t "=ralph-3ds-wait" 2>/dev/null && echo WAIT=up || echo WAIT=down
  lms ps 2>/dev/null | grep -q "^ralph-3ds " && echo MODEL=loaded || echo MODEL=missing
  curl -sf --max-time 5 http://127.0.0.1:1234/v1/models >/dev/null && echo API=up || echo API=down
  echo PRD_AGE=$(( $(date +%s) - $(stat -f %m "$B/progress.txt" 2>/dev/null || echo 0) ))
  tail -40 "$B/progress.txt" 2>/dev/null | grep -c "verify_ok: False" | sed "s/^/RECENT_FAILS=/"
') || { echo "watchdog: studio unreachable"; exit 0; }

LOOP=$(echo "$VITALS" | grep -o 'LOOP=[a-z]*' | cut -d= -f2)
WAIT=$(echo "$VITALS" | grep -o 'WAIT=[a-z]*' | cut -d= -f2)
MODEL=$(echo "$VITALS" | grep -o 'MODEL=[a-z]*' | cut -d= -f2)
API=$(echo "$VITALS" | grep -o 'API=[a-z]*' | cut -d= -f2)
PRD_AGE=$(echo "$VITALS" | grep -o 'PRD_AGE=[0-9]*' | cut -d= -f2)

# Silent until the LOOP session has been seen up at least once (queue phase is
# wait-for-slot's job; the model is loaded by launch.sh at claim time, not us).
if [ "$LOOP" = "down" ] && [ ! -f "$STATE.started" ]; then exit 0; fi
[ "$LOOP" = "up" ] && touch "$STATE.started"

INCIDENT=""
# Loop dead AND no queue watcher alive to bring it back = incident.
[ "$LOOP" = "down" ] && [ "$WAIT" = "down" ] && INCIDENT="loop-and-wait-down"
[ "$LOOP" = "up" ] && [ "$MODEL" = "missing" ] && INCIDENT="$INCIDENT model-missing"
[ "$API" = "down" ] && INCIDENT="$INCIDENT api-down"
# Dead-man: 45 min without a progress touch while loop is up (> max iter 30m).
[ "$LOOP" = "up" ] && [ "${PRD_AGE:-0}" -gt 2700 ] && INCIDENT="$INCIDENT stale-${PRD_AGE}s"

LAST=$(cat "$STATE" 2>/dev/null || true)
if [ -z "$INCIDENT" ]; then rm -f "$STATE"; exit 0; fi
[ "$INCIDENT" = "$LAST" ] && exit 0   # same incident: already handled once
echo "$INCIDENT" > "$STATE"

FIXES=""
if [ "$LOOP" = "up" ] && [ "$MODEL" = "missing" ]; then
  "${SSH[@]}" 'export PATH="$HOME/.lmstudio/bin:$PATH"; lms load qwen3.8-27b@8bit --identifier ralph-3ds --context-length 131072 -y' \
    && FIXES="$FIXES reloaded-model" || FIXES="$FIXES model-reload-FAILED"
fi
# HARD RULE: never start the supervisor directly (that would skip the slot
# claim). Restart the QUEUED path — queued-start claims before launching.
if [ "$LOOP" = "down" ] && [ "$WAIT" = "down" ] && [ -f "$STATE.started" ]; then
  "${SSH[@]}" 'export PATH=/opt/homebrew/bin:$PATH; tmux new -d -s ralph-3ds-wait "caffeinate -i bash $HOME/3ds-qwen-remake/scripts/ops/queued-start.sh >> $HOME/3ds-qwen-remake/ralph.log 2>&1"' \
    && FIXES="$FIXES restarted-queued-start" || FIXES="$FIXES queued-start-restart-FAILED"
fi

"${SSH[@]}" "cd ~/3ds-qwen-remake && scripts/ops/notify.sh '🚨 watchdog: $INCIDENT | fixes:${FIXES:- none} | $(echo "$VITALS" | tr "\n" " ")'" || true
