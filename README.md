# Ashen Rift

Ashen Rift is an original, low-poly action-RPG homebrew demo for the New Nintendo 3DS family. It is a from-scratch fan demake experiment inspired by the feeling of modern dark-fantasy games, not a port or redistribution of any commercial title.

The playable sequence connects three memory-bounded zones: a sunken interior, an outdoor reveal with an NPC encounter, and a boss arena. It includes third-person movement, C-stick camera control, lock-on, stamina combat, healing, dialogue, a scripted boss, dual-screen UI, generated original audio, and performance diagnostics.

## Status

- Native fixed-step gameplay, three-zone streaming state, combat, lifecycle handling, dual-screen UI, and NDSP audio are implemented.
- The original-content pipeline includes generated asset IDs, per-zone manifests and budgets, a Blender-editable scene source, 15-bone rigid animation clips, and an RGB565 `tex3ds` atlas.
- Static props use generated fixed-size data, an indexed VBO, coarse-grid and view/distance culling, baked colors, a directional tint, fog-gate masking, blob shadows, and distant panorama panels.
- Repository policy audit, asset validation, deterministic host smoke flows, native artifact verification, and private GitHub Actions are implemented; see [BUILD_EVIDENCE.md](docs/BUILD_EVIDENCE.md).
- Physical New Nintendo 3DS-family verification remains required. A local build or emulator boot is deliberately not called completion.

## Architecture

`GameApp` owns fixed-step input/lifecycle, `GameSession` owns title/pause/suspend state, `ZoneManager` owns preload/enter/unload handoffs, `PlayerController` and `BossController` own their independent combat state machines, `Renderer` owns citro3d/citro2d output and counters, `AudioStreamer` owns NDSP double buffers, and generated `AssetRegistry` data connects runtime assets to each zone without per-frame allocation.

## Build

Install devkitPro's `3ds-dev` group, then run:

```sh
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=/opt/devkitpro/devkitARM
make validate-assets
make test-host
make verify-build
make package-sd
```

The native output is `elden-ring-3ds-demake.3dsx`. `make package-sd` also creates a verified `dist/ashen-rift-sd-bundle.zip` whose paths can be copied directly to the SD-card root. See [BUILDING.md](docs/BUILDING.md) for installation and netloading.

## Original-asset policy

The repository must not contain extracted game models, textures, music, dialogue, code, console-unique data, SD-card backups, or credentials. Runtime content is original and tracked through `assets/manifest.json` and [ASSET_PROVENANCE.md](docs/ASSET_PROVENANCE.md).

## Disclaimer

This is an unofficial, not-for-sale fan project. It is not affiliated with or endorsed by Nintendo, FromSoftware, Bandai Namco, or any other publisher or platform holder. The repository remains private during v1 development and has no open-source license.
