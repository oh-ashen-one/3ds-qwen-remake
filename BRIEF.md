# ASHEN RIFT — enhancement bible

One line: a dark-fantasy action-RPG vertical slice on original 3DS hardware
that should feel like a lost commercial cartridge, not a homebrew demo.

## Quality bars (non-negotiable)

- Old-3DS (CTR-001) is the target. Frame budget: 30 fps floor in every zone,
  measured by the in-game diagnostics counters. A change that drops the floor
  is wrong no matter how pretty.
- Linear-memory headroom stays positive in every zone; the per-zone budgets in
  asset-budget-report.json are law.
- The build is always shippable: every commit builds a valid 3DSX.

## Feel numbers (the existing game's contract — keep or improve, never regress)

- Fixed-step simulation; input latency never worse than one tick.
- Stamina combat: attacks commit (no cancel-spam), dodges have i-frames,
  telegraphs are readable at 3DS screen size (≥ 0.5 s wind-up on heavies).
- Camera: D-pad orbit + lock-on; never clips through world; never jitters
  on horseback.
- Bosses have distinct silhouettes, one signature attack each, and clear
  recovery windows after their combos.

## Palette & atmosphere

- Interior/arena: ember-and-ash — deep umber shadows, warm orange rim glow.
- Sunlit Reach: Japanese-alpine dawn — cool sky, warm grass, layered blue
  mountain silhouettes, drifting cloud banks.
- Fog is the horizon color of its zone, never gray. Distance fades into
  atmosphere, not into black.
- Dialogue and UI text: bone-white on near-black, thin frames, no rounded
  bubbles, no drop shadows.

## What "better" means, in priority order

1. Feel: hit-pause, camera weight, dodge/parry timing, boss readability.
2. World density: more props/wildlife/ambient motion inside zone budgets.
3. Encounter variety: enemy behaviors, not new enemy meshes.
4. Audio: stingers on boss phases, ambient beds per zone.
5. Polish: HUD legibility, transition masks, title screen.

## Slop list — never do these

- No screen-space vignettes, no bloom fakes, no lens flares.
- No generic fantasy names (no "Shadowlord", "Darkblade", "Netherrealm").
- No new UI fonts, no emoji, no ALL-CAPS menu labels.
- No difficulty inflation as "improvement" — better telegraphs beat more HP.
- No feature that can't be verified by the build+test chain or a probe.
