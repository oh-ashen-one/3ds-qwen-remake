# Ashen Rift

Ashen Rift is an original, low-poly action-RPG homebrew demo rebuilt for the original Nintendo 3DS (CTR-001). It is a from-scratch fan demake experiment inspired by the feeling of modern dark-fantasy games, not a port or redistribution of any commercial title.

The playable sequence connects six memory-bounded zones: a sunken interior, an outdoor reveal with an NPC encounter, the Warden arena, the post-boss Sunlit Reach, an eastern hog ravine, and a western horseback ascent through clouds to an ogre summit. It includes third-person movement, D-pad camera control, lock-on, stamina combat, healing, RomFS-backed text dialogue, three distinct bosses, rideable horses, dual-screen maps and controls, original audio, and performance diagnostics.

The complete project contract is preserved in [MASTER_GOAL_PROMPT.md](MASTER_GOAL_PROMPT.md); [GOAL_PROMPT.md](GOAL_PROMPT.md) is its reusable sub-4,000-character `/goal` launcher.

See [OPEN_SOURCE_CREDITS.md](OPEN_SOURCE_CREDITS.md) for the complete,
linked list of console-enablement projects, homebrew applications, development
libraries, asset tools, emulator, and CI infrastructure used during the build.

## Status

- Native fixed-step gameplay, six-zone streaming state, combat, lifecycle handling, dual-screen UI, and NDSP audio are implemented.
- Defeating the Ashen Warden now opens into the Sunlit Reach: a broad Japanese-alpine valley with a winding river and snowmelt tributary, bridge and ford, cedar slopes, three flower families, layered snowcapped mountains, drifting cloud banks, running wildlife, and three rideable horses.
- The field now branches east into Twinfang Ravine and its charging giant hog, or west into Cloudbreak Ascent: a horse-scale ragged climb through moving cloud layers to a high plateau and a horned mountain ogre with telegraphed violet rune magic. Both branches return to the field and preserve defeated bosses on revisit.
- Red and cloudstone gates now mark those branches in the world. Every branch handoff preloads the destination behind a departure/arrival mask, and zone-title reveals replace the old edge teleport.
- Exploration and boss tracks fade between Ashen Deep Hall, Valley After Dawn, and Ashen Gate. Boss-area deaths return to the latest grace with restored resources; branch victories restore health and one flask so the extended showcase remains practical in one take.
- The lighting and material palette were lifted for original-3DS visibility, and the boss now has lower health, slower telegraphs, gentler pursuit, smaller hit zones, and clearer recovery windows.
- The original-content pipeline includes generated asset IDs, per-zone manifests and budgets, independently loadable RomFS scene blobs, a Blender-editable scene source, 15-bone rigid animation clips, and an RGB565 `tex3ds` atlas.
- Static props use generated fixed-size data, an indexed VBO, coarse-grid and view/distance culling, baked colors, a directional tint, fog-gate masking, blob shadows, and distant panorama panels.
- Repository policy audit, asset validation, deterministic host smoke flows, native artifact verification, and GitHub Actions are implemented; see [BUILD_EVIDENCE.md](docs/BUILD_EVIDENCE.md).
- The requirement-by-requirement state is maintained in [COMPLETION_AUDIT.md](docs/COMPLETION_AUDIT.md), with both the intended 7:25 core route and extended branch showcase in [PLAYTHROUGH_ROUTE.md](docs/PLAYTHROUGH_ROUTE.md).
- The physical gate has a deliberately failing-until-tested [hardware report template](docs/HARDWARE_REPORT_TEMPLATE.json) and validator; see [HARDWARE_TEST.md](docs/HARDWARE_TEST.md).
- Physical verification on the user's Japanese original Nintendo 3DS remains required. A local build or emulator boot is deliberately not called completion.

## Architecture

`GameApp` owns fixed-step input/lifecycle, `GameSession` owns title/pause/suspend state, `ZoneManager` owns logical preload/masked-enter/unload handoffs and grace checkpoints, `ZoneResources` mirrors that mask with real RomFS loads and linear-memory frees, `PlayerController` and `BossController` own their independent combat state machines, `Renderer` owns citro3d/citro2d output and counters, `AudioStreamer` owns NDSP double buffers and track fades, and generated `AssetRegistry` data connects runtime assets to each zone without per-frame allocation.

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

The Sunlit Reach now has its own user-supplied original track, **Valley After
Dawn**. Its music-generation brief is preserved in
[SUNO_SUNLIT_REACH_PROMPT.md](docs/SUNO_SUNLIT_REACH_PROMPT.md).

## Install on a homebrewed 3DS

1. Build the verified SD bundle with `make package-sd`, or download a published build artifact.
2. Copy the bundle contents to the root of the console's SD card. This places the app at `sdmc:/3ds/elden-ring-3ds-demake/elden-ring-3ds-demake.3dsx`.
3. Open Homebrew Launcher and select **Ashen Rift**.

The console must already have a current boot9strap/Luma3DS homebrew setup. Follow the current [3DS Hacks Guide](https://3ds.hacks.guide/) for the exact console model and firmware; never copy another console's NAND, keys, or `Nintendo 3DS` directory.

## Original-asset policy

The repository must not contain extracted game models, textures, music, dialogue, code, console-unique data, SD-card backups, or credentials. Runtime content is original and tracked through `assets/manifest.json` and [ASSET_PROVENANCE.md](docs/ASSET_PROVENANCE.md).

## Disclaimer

This is an unofficial, not-for-sale fan project. It is not affiliated with or endorsed by Nintendo, FromSoftware, Bandai Namco, or any other publisher or platform holder. The original source code and assets in this repository are available under the [MIT License](LICENSE); third-party tools and platform components retain their own licenses.
