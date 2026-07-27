# Master Goal Prompt: Elden Ring-Inspired Original Nintendo 3DS Homebrew Demo

## Mission

Own this project through a verified, playable build on the user's physical Japanese original Nintendo 3DS (CTR-001). Maintain the public GitHub repository, build a native 3DS homebrew vertical slice, establish a rapid development-to-3DS deployment loop, and preserve real development footage/evidence so the user can make a YouTube video with the same narrative strength as the supplied PS1 Elden Ring transcript.

The result is not a port of the commercial game. It is an unofficial, from-scratch fan demake: original low-poly art, original animations, original dialogue, original music/SFX, and no extracted FromSoftware assets or code.

## Verified Pivot State (2026-07-19)

- Local workspace: the repository root; no machine-specific absolute path belongs in tracked files.
- Git/public GitHub: `oh-ashen-one/elden-ring-3ds-demake` exists and uses `main` as its release branch.
- Local toolchain: the official devkitPro 3DS stack is installed and has produced a verified `.3dsx` and SD-root package.
- Hardware: the user's photograph confirms an aqua Japanese original Nintendo 3DS, model family CTR-001, on System Settings `11.17.0-50J`; boot9strap and Luma3DS are installed, while the game-specific physical acceptance report remains pending.
- SD card: the user's 32 GB FAT32 card was health-checked and the app bundle was installed without disturbing its existing `Nintendo 3DS` data.
- User-approved pivot: replace the former New-3DS-only baseline with the exact original Nintendo 3DS. Preserve the full vertical slice and physical acceptance gate; do not solve compatibility by shrinking the promised game.

Re-verify all of these facts before relying on them because local, GitHub, package, and hardware state may change.

## Non-Negotiable Outcomes

1. Maintain `oh-ashen-one/elden-ring-3ds-demake` as a **public** GitHub repository with an MIT license, privacy-safe history, and no secrets, personal paths, console-unique data, or backups.
2. Produce a self-contained Homebrew Launcher `.3dsx` built with the official devkitPro stack.
3. Implement a coherent 5–8 minute sequence: title screen, enclosed opening room, animated doorway/lift reveal, outdoor vista, short NPC interaction, fog-gate/transition, third-person boss fight, victory/death, and a golden transition into a broad, bright post-boss grassland with a rideable horse, sun, clouds, layered mountains, and a distant waystone.
4. Support physical-device iteration through both:
   - Wi-Fi netloading: Homebrew Launcher netloader + `3dslink`.
   - Persistent offline play: application bundle under `sdmc:/3ds/elden-ring-3ds-demake/`.
5. Do not call the project complete until it has passed the physical-hardware acceptance gate. Emulator success, compilation, or a successful transfer alone is not proof.
6. Preserve a development log and real milestone captures rather than fabricating broken-build footage after completion.

## Product Scope

### Required gameplay

- Top screen: native third-person 3D game at 400x240.
- Bottom screen: quick items, controls/context, and a toggleable debug/performance display.
- Original-3DS controls: Circle Pad movement; D-pad left/right camera; D-pad up/down item selection; L lock-on; R light attack; Y heavy attack; B dodge/sprint; A interact/confirm; X heal; Start pause; Select exit; lower-right touchscreen tap toggles diagnostics.
- New-family C-stick/ZR input may remain as optional compatibility, but no required gameplay, performance, UI, or acceptance path may depend on New-family controls or CPU speedup.
- Player states: idle, locomotion, attack, dodge, hurt, heal, interact, death, and victory lockout.
- Health, stamina, healing charges, damage, dodge invulnerability, target lock, simple hit reactions, and restart flow.
- One readable boss with a compact state machine, multiple telegraphed attacks, collision/hitboxes, health bar, name card, victory state, and arena reset.
- One short NPC interaction with subtitles and original voice/audio or intentionally text-only presentation if audio budget demands it.

### Explicitly out of scope for v1

- Open-world streaming beyond the three authored zones.
- Character creator, equipment/inventory system, save files, quests, multiplayer, multiple bosses, ragdolls, dynamic shadows, or a public binary release.
- Stereoscopic 3D as an acceptance requirement. Treat it only as a post-v1 stretch goal after performance is proven.

## Technical Architecture

- Native C++17 with devkitARM; disable RTTI and exceptions; avoid per-frame heap allocation.
- `libctru`: lifecycle, HID, services, filesystem/RomFS, model detection, memory reporting, and NDSP audio.
- `citro3d`: PICA200 rendering, render targets, shaders, VBOs, textures, and draw submission.
- `citro2d`: HUD, text, bottom-screen UI, dialogue, and debug overlay.
- `tex3ds`: offline texture conversion and atlases.
- Build with a devkitPro-style Makefile and generate `.elf`, `.map`, `.smdh`, and `.3dsx` outputs.
- Compile for the original 3DS ARM11 MPCore baseline and retain the 30 FPS fixed step. Use model detection for truthful diagnostics and optional newer-family enhancements only; never reject CTR-001 at startup.

Use clear subsystem boundaries:

- `GameApp`: initialize, fixed-step update, render, suspend/resume, and shutdown.
- `ZoneManager`: load, enter, update, exit, unload, and transition between interior, vista/NPC, and boss arena.
- `Renderer`: static meshes, materials, camera, panorama, culling, lighting, sprites/UI, and performance counters.
- `PlayerController`: input, movement, stamina, combat states, damage, and lock-on.
- `BossController`: encounter lifecycle, deterministic state machine, attacks, recovery, damage, and reset.
- `AudioStreamer`: double-buffered streaming plus resident short SFX.
- `AssetRegistry`: generated IDs and metadata for meshes, textures, animation clips, audio, and zone manifests.

## Rendering, Memory, and Content Pipeline

- Use compact static VBOs, indexed low-poly meshes, texture atlases, baked vertex color/lighting, one simple directional contribution, fog, blob shadows, and a panoramic distant backdrop.
- Use distance and view-frustum culling plus a coarse spatial grid for props.
- Split content into three independently loadable zones. The closed doorway hides the outdoor load; unload the interior after the reveal. The fog gate/fade hides the boss arena handoff.
- Use a rigid 12–16 bone hierarchy with per-bone transform clips; avoid expensive blended skinning for v1.
- Author assets in Blender and convert them offline into compact runtime data. Keep source assets, conversion scripts, generated manifests, and provenance records separate.
- Package runtime content in RomFS so netloading and SD installation can use one `.3dsx`.
- Stream original mono dialogue/music at a conservative rate through NDSP double buffers; keep only short combat SFX resident.
- Add runtime counters for frame time, draw calls, visible/culled objects, zone memory, linear memory, and audio underruns.

## Repository and Automation

- Add a concise README, build/setup guide, controls, fan-project disclaimer, asset-provenance manifest, `.gitignore`, milestone log, test instructions, and hardware-test checklist.
- Keep the repository public under its MIT license and audit the current tree plus reachable history before every release.
- Add `make`, `make clean`, host-test, asset-validation, and `make run IP=<3DS-IP>` workflows.
- Add GitHub Actions that use the official devkitPro devkitARM container, build reproducibly, run host tests/asset validation, and retain `.3dsx`, `.elf`/map, and budget reports without personal build paths.
- Commit in coherent milestones and push them. Do not include copyrighted source media, credentials, SD-card backups, console-unique files, or generated build directories.

## Video-Production Evidence

Use the reference transcript's story architecture without copying its wording:

1. Hook: the user's real aqua original Nintendo 3DS visibly running the demo.
2. Hardware contrast and why this is unreasonable.
3. First official graphics example becoming an ugly third-person prototype.
4. Broken model/animation/camera and an overly ambitious early arena attempt.
5. The smaller connected vertical slice and the doorway/zone-memory trick.
6. Culling, low-poly assets, lighting, panorama, audio, and dual-screen UI.
7. Lock-on, combat, HUD, and boss development.
8. Local/emulator completion, followed by the decisive physical-hardware test.
9. Honest reflection on constraints, cuts, and what actually worked.

Maintain a milestone log with build/commit identifier, what changed, bugs visible, performance numbers, and which footage the user should capture. Never claim “running on real hardware” until an uncut physical-console test proves it.

## Testing and Acceptance

### Automated/local

- Host tests for math, collision, animation sampling, player state transitions, stamina/damage, boss transitions, lock-on target loss, and zone handoffs.
- Asset validation for missing references, incompatible formats, oversized textures/audio, invalid clips, uncredited assets, and per-zone budgets.
- Build validation for clean checkout reproduction and embedded RomFS/SMDH metadata.
- Smoke flows: boot, title-to-game, dialogue, zone transition, death/restart, victory/restart, pause/resume, and repeated arena reset.

### Physical hardware

- Confirm the exact CTR-001 model/region, System Settings version, Luma/Homebrew Launcher state, SD-card readiness, and network access with the user.
- Prove Wi-Fi launch through `3dslink` and persistent launch from the SD card without the Mac.
- Run three consecutive cold-boot full playthroughs.
- Target 30 FPS with no sustained drops below 24 FPS, no crashes, no audio underruns, no visible loading outside masked transitions, and at least 20% measured memory headroom at each zone's peak.
- Test sleep/wake, Homebrew Launcher return, failed/aborted dialogue, lost lock-on target, player death during boss attacks, and repeated reloads.
- Capture an uncut proof shot that includes the physical console, both screens, Homebrew Launcher, application launch, and gameplay.

## Execution Rules

- Work autonomously through every safe local and GitHub step authorized above.
- Preserve dirty/unrelated user work and inspect before changing anything.
- Use current official documentation for devkitPro and current 3DS homebrew setup; do not rely on stale exploit instructions.
- Prefer the smallest working implementation, measure on hardware, then add detail. Do not start with the finished boss arena.
- If package installation needs an administrator password, or the physical-device gate needs console interaction, report the exact minimal user action and continue every other unblocked task first.
- Do not silently weaken the required vertical slice or the physical-hardware completion gate.
- Report evidence, not optimistic status. A build, emulator screenshot, HTTP response, or transfer command is not equivalent to successful gameplay on the console.

## Definition of Done

The goal is complete only when the public, privacy-audited repository and CI are healthy; the original-asset vertical slice is fully playable; the `.3dsx` works through both Wi-Fi netloading and persistent SD installation; the user's physical Japanese original Nintendo 3DS (CTR-001) passes the three-playthrough performance/stability gate; and the repository contains the build artifact references, test evidence, hardware report, controls, milestone footage index, and narration/shot outline needed for the user's video.
