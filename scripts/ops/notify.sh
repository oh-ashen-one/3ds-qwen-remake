#!/bin/bash
# notify.sh <text> [image ...] — Telegram via Studio Hermes credentials.
# RUNS ON THE STUDIO ONLY. Token stays in ~/.hermes/.env, never in output.
set -uo pipefail
TEXT=${1:?usage: notify.sh <text> [image ...]}
shift || true
HERMES_PY="$HOME/.hermes/hermes-agent/venv/bin/python"

# Text through hermes send (no LLM, reuses bot credentials).
printf '%s\n' "$TEXT" | "$HERMES_PY" -m hermes_cli.main send \
  --to telegram:ashen --subject '[3ds] ⚔️' --quiet || echo "notify: text send failed"

# Photos via Bot API (hermes send has no attachments).
if [ $# -gt 0 ]; then
  set +x
  TOKEN=$(grep '^TELEGRAM_BOT_TOKEN=' ~/.hermes/.env | cut -d= -f2- | tr -d '"')
  CHAT=$(grep '^TELEGRAM_HOME_CHANNEL=' ~/.hermes/.env | cut -d= -f2- | tr -d '"')
  for img in "$@"; do
    [ -f "$img" ] || continue
    curl -sS -o /dev/null -F chat_id="$CHAT" -F photo=@"$img" \
      "https://api.telegram.org/bot${TOKEN}/sendPhoto" || echo "notify: photo failed $img"
  done
fi
