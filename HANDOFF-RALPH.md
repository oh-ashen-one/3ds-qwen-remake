# Ralph loop runbook — 3ds-qwen-remake

Any fresh manager resumes from this file + prd.json + progress.txt + git log.
The playbook is `oh-ashen-one/gaming-ralph-loops` — laws there are binding.

## State (2026-08-16, staged, NOT started)

- Studio clone: `studio:~/3ds-qwen-remake` (ssh alias `studio`; loop runs THERE, never over ssh).
- Toolchain: self-contained at `~/.local/share/elden-ring-3ds-devkit` (tools/env.sh points at it). `/opt/devkitpro` on the studio is GBA-only — wrong one.
- Baseline verified green on the studio: validate-assets, test-host, 3DSX build (33,166,632 bytes, ~2s), verify-build.
- Model: LM Studio `qwen3.8-27b@8bit` loaded as dedicated instance `ralph-3ds` by launch.sh. NEVER touch instances `fifa-ralph`, `ralph-showcase`, or clipper aliases.
- Queued behind fifa: `scripts/ops/wait-for-fifa.sh` polls `~/fifa-2026/scripts/ralph/prd.json` unpassed==0 (or fifa tmux gone x3), then execs launch.sh.

## Start it (human go required)

```bash
ssh studio
cd ~/3ds-qwen-remake && git pull
tmux new -d -s ralph-3ds-wait 'caffeinate -i scripts/ops/wait-for-fifa.sh >> ralph.log 2>&1'   # queued start
# or immediately: scripts/ops/launch.sh
```

Then on the laptop, install the watchdog cron (*/5): `scripts/ops/watchdog.sh` (external box on purpose — it still reports when the studio wedges).

## Ops

- Progress: `scripts/ops/tg-status.sh` (Telegram poke), `tail -f ralph.log`, `tmux attach -t ralph-3ds`.
- Phase gate: supervisor parks at exit 42 when a phase's stories all pass. Manager MUST enrich next-phase stories (see prd notes) then `echo N > PHASE`.
- Rut: read scripts/ralph/LAST_VERIFY.txt + LAST_REPLY.md + LAST_THINKING.md, then apply the rut playbook (gaming-ralph-loops/PROMPTS.md) — tighten the story, never author code.
- Kill order: watchdog cron first, then tmux ralph-3ds-wait/ralph-3ds, then pgrep -f qwen_iteration.

## Manager TODO before PHASE 1

- Survey source/renderer.cpp (1476 lines) and source/core.cpp (904): pick the exact functions for US-010/US-011, write them into the story descriptions, and derive `protect` lists by grepping the CURRENT headers (failure #29: never from convention).
- Phase 2 stories are seeds — enrich with exact code sites after phase 1 lands; US-021 needs a stinger PCM staged into assets by the manager first.
- Critique phase (CRITIC.md) needs a screenshot path for 3DS: no Citra/Azahar on the studio yet. Options: install Azahar headless-ish captures, or 3dslink to real hardware + photo. Decide before calling anything "done".

## Design decisions already made

- prd/QWEN/BRIEF live at repo root (driftwood layout); runner is driftwood's with a 3DS write fence: model writes only source/, include/ (never generated/), assets/*.json, gfx/*.t3s|.ppm.
- Verify backbone: `tools/ralph-verify.sh` = validate-assets → test-host → make → verify-build (CI parity). `quick` skips verify-build.
- Push to GitHub stays manual — the loop commits locally only.
