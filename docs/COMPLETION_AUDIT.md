# Master-contract completion audit

Authoritative contract: `MASTER_GOAL_PROMPT.md`. This audit separates implemented/local evidence from claims that require the user's physical New Nintendo 3DS-family console.

Status meanings:

- **Local pass** — current source or a deterministic clean test directly proves the requirement.
- **Implemented, hardware pending** — the path exists, but emulator/build evidence cannot prove real-device behavior.
- **Pending** — the required external evidence does not exist yet.

## Repository, policy, and automation

| Requirement | Status | Authoritative evidence |
| --- | --- | --- |
| Private `oh-ashen-one/elden-ring-3ds-demake` repository and `origin` | Local pass | Live GitHub repository metadata plus `git remote -v`; private CI/PRs are recorded in `BUILD_EVIDENCE.md` |
| No extracted commercial assets, credentials, console-unique files, SD backups, release package, or public license | Local pass | `make audit-repo`; `assets/manifest.json`; `ASSET_PROVENANCE.md`; no license file |
| Official devkitPro native C++17 build | Local pass | `Makefile`, devkitARM flags, libctru/citro3d/citro2d/tex3ds use, clean `make verify-build` |
| Private CI builds/tests/verifies and retains artifacts | Local pass | `.github/workflows/build.yml`; green GitHub Actions runs and downloaded private artifacts in `BUILD_EVIDENCE.md` |
| Netload and persistent SD workflows exist | Implemented, hardware pending | `make check-netload IP=...` validates address/artifact/tool discovery; `make run IP=...`; deterministic `make package-sd`; SD-root path and embedded source/hash validation |

## Playable vertical slice

| Requirement | Status | Authoritative evidence |
| --- | --- | --- |
| Title, enclosed opening, animated door reveal, outdoor vista, NPC, fog transition, boss, death/victory, restart | Local pass for state flow | `GameSession`, `ZoneManager`, `Renderer`, and deterministic host smoke flows |
| Circle Pad/C-stick plus L/R/ZR/B/A/X/D-pad/Start controls | Implemented, hardware pending | `GameApp::readInput`, `CONTROLS.md`, host state tests; ergonomics require the console |
| Player combat states, health, stamina, flasks, invulnerability, lock-on, hit reactions | Local pass | `PlayerController`, rigid-pose sampler, combat and edge-case host tests |
| Boss with multiple telegraphed attacks, hitboxes, health/name UI, victory/reset | Local pass for logic | `BossController`, arena renderer, deterministic slash/slam/death/victory/reset tests |
| Original text-only NPC presentation | Local pass | UTF-8 `romfs/dialogue/keeper.txt` is validated and loaded into a fixed renderer buffer from RomFS |
| Coherent 5–8-minute completion | Implemented route, hardware pending | `PLAYTHROUGH_ROUTE.md` targets 6:40; the checklist requires three measured physical runs and forbids idle padding |

## Runtime architecture and content pipeline

| Requirement | Status | Authoritative evidence |
| --- | --- | --- |
| Clear GameApp/ZoneManager/Renderer/Player/Boss/Audio/Asset boundaries | Local pass | Native headers/sources and required linked-symbol verification |
| Three independently loadable zones with masked overlap and prior-zone free | Local pass for allocation logic | `ZoneResources`, ASZN blobs, host parser/residency tests, native map without host-only all-zone arrays |
| Original low-poly/Blender/rigid-animation/texture pipeline | Local pass | 53 authored props, editable `.blend` plus fingerprints, 15-bone clips, RGB565 T3X, generated registry/blobs |
| RomFS content and NDSP double-buffered original audio | Local pass for build/runtime path | Embedded marker/link verification and `AudioStreamer`; audible continuity/underruns remain hardware-only |
| No per-frame heap allocation in game loop | Local pass by inspection | Fixed members/buffers; transition-only linear allocations; preallocated renderer/audio resources |
| Frame/draw/culling/zone/linear-memory/audio counters | Implemented, hardware pending | Bottom-screen diagnostics and verifier; performance values require physical sampling |

## Verification and evidence

| Requirement | Status | Authoritative evidence |
| --- | --- | --- |
| Host tests for math, collision, animation, combat, boss, lock-on, lifecycle, zone handoffs, resets | Local pass | `make test-host` and `tests/core_tests.cpp` |
| Asset format/reference/provenance/budget validation | Local pass | `make validate-assets`; deterministic registry, scene, blend, texture, audio, dialogue, and zone checks |
| Clean native artifact/SMDH/RomFS/link/package verification | Local pass | `make clean`, `make verify-build`, `make package-sd`, `3dsxdump`, hashes in `BUILD_EVIDENCE.md` |
| Honest milestone and video-production workflow | Local pass for documentation | `MILESTONES.md`, `VIDEO_OUTLINE.md`, `PLAYTHROUGH_ROUTE.md`; real footage must still be captured as milestones occur |
| Wi-Fi netload, offline SD launch, sleep/wake, Select return | Pending | Requires connected console and `HARDWARE_TEST.md` evidence |
| Three cold-boot 5–8-minute runs at required FPS/memory/audio/stability thresholds | Pending | `HARDWARE_REPORT_TEMPLATE.json` and `validate_hardware_report.py` encode the gate, but the measured `HARDWARE_REPORT.json` does not exist; this is the controlling completion blocker |
| Uncut physical-console proof shot | Pending | Must show console body, both screens, Homebrew Launcher, launch, and gameplay |

## Current completion decision

The repository is a buildable, packaged hardware candidate, not a completed goal. Completion remains prohibited until all physical rows above pass and their measured evidence is committed. Emulator rendering, CI success, packaging, or transfer alone cannot change that decision.
