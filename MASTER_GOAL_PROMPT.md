# Master Goal Prompt: Elden Ring-Inspired New Nintendo 3DS Homebrew Demo

## Mission

Own this project from an empty local Git repository through a verified, playable build on the user's physical New Nintendo 3DS. Create a private GitHub repository, build a native 3DS homebrew vertical slice, establish a rapid Mac-to-3DS deployment loop, and preserve real development footage/evidence so the user can make a YouTube video with the same narrative strength as the supplied PS1 Elden Ring transcript.

The result is not a port of the commercial game. It is an unofficial, from-scratch fan demake: original low-poly art, original animations, original dialogue, original music/SFX, and no extracted FromSoftware assets or code.

## Verified Starting State

- Local workspace: `PROJECT_ROOT`
- Git: empty repository on `main`, with no commits and no remote.
- GitHub: authenticated as `oh-ashen-one`; the intended private name `oh-ashen-one/elden-ring-3ds-demake` was available when planning was performed.
- Local toolchain: devkitARM exists under `/opt/devkitpro`; the 3DS-specific `3ds-dev` packages (`libctru`, `citro3d`, `citro2d`, `tex3ds`, `3dslink`, and related tools) were not installed.
- Hardware: no 3DS is connected now. Build everything possible locally, then stop at the real-device boundary and request the console/model/homebrew/network details needed for testing.
- Intended hardware baseline: New Nintendo 3DS, New Nintendo 3DS XL, or New Nintendo 2DS XL.

Re-verify all of these facts before relying on them because local, GitHub, package, and hardware state may change.

## Non-Negotiable Outcomes

1. Create `oh-ashen-one/elden-ring-3ds-demake` as a **private** GitHub repository and connect it as `origin` without disturbing unrelated user files.
2. Produce a self-contained Homebrew Launcher `.3dsx` built with the official devkitPro stack.
3. Implement a coherent 5–8 minute sequence: title screen, enclosed opening room, animated doorway/lift reveal, outdoor vista, short NPC interaction, fog-gate/transition, third-person boss fight, victory/death, and restart.
4. Support physical-device iteration through both:
   - Wi-Fi netloading: Homebrew Launcher netloader + `3dslink`.
   - Persistent offline play: application bundle under `sdmc:/3ds/elden-ring-3ds-demake/`.
5. Do not call the project complete until it has passed the physical-hardware acceptance gate. Emulator success, compilation, or a successful transfer alone is not proof.
6. Preserve a development log and real milestone captures rather than fabricating broken-build footage after completion.

## Product Scope

### Required gameplay

- Top screen: native third-person 3D game at 400x240.
- Bottom screen: quick items, controls/context, and a toggleable debug/performance display.
- Circle Pad movement; C-stick camera; L lock-on; R light attack; ZR heavy attack; B dodge/sprint; A interact/confirm; X heal; D-pad item selection; Start pause.
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
- Keep the repository private and omit an open-source license for v1.
- Add `make`, `make clean`, host-test, asset-validation, and `make run IP=<3DS-IP>` workflows.
- Add GitHub Actions that use the official devkitPro devkitARM container, build reproducibly, run host tests/asset validation, and retain private `.3dsx`, `.elf`/map, and budget reports.
- Commit in coherent milestones and push them. Do not include copyrighted source media, credentials, SD-card backups, console-unique files, or generated build directories.

## Video-Production Evidence

Use the reference transcript's story architecture without copying its wording:

1. Hook: a real New Nintendo 3DS visibly running the demo.
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

- Confirm the exact New 3DS-family model, Luma/Homebrew Launcher state, SD-card readiness, and network access with the user.
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

The goal is complete only when the private repository and CI are healthy; the original-asset vertical slice is fully playable; the `.3dsx` works through both Wi-Fi netloading and persistent SD installation; the physical New Nintendo 3DS passes the three-playthrough performance/stability gate; and the repository contains the build artifact references, test evidence, hardware report, controls, milestone footage index, and narration/shot outline needed for the user's video.
