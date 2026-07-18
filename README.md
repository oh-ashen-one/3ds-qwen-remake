# Ashen Rift

Ashen Rift is an original, low-poly action-RPG homebrew demo for the New Nintendo 3DS family. It is a from-scratch fan demake experiment inspired by the feeling of modern dark-fantasy games, not a port or redistribution of any commercial title.

The playable sequence connects three memory-bounded zones: a sunken interior, an outdoor reveal with an NPC encounter, and a boss arena. It includes third-person movement, C-stick camera control, lock-on, stamina combat, healing, dialogue, a scripted boss, dual-screen UI, generated original audio, and performance diagnostics.

## Status

- Native gameplay simulation and procedural renderer implemented.
- Host-side deterministic tests and asset validation implemented.
- Physical New Nintendo 3DS verification is required before this project can be called complete.

## Build

Install devkitPro's `3ds-dev` group, then run:

```sh
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=/opt/devkitpro/devkitARM
make validate-assets
make test-host
make
```

The output is `elden-ring-3ds-demake.3dsx`. See [BUILDING.md](docs/BUILDING.md) for installation and netloading.

## Original-asset policy

The repository must not contain extracted game models, textures, music, dialogue, code, console-unique data, SD-card backups, or credentials. Runtime content is original and tracked through `assets/manifest.json` and [ASSET_PROVENANCE.md](docs/ASSET_PROVENANCE.md).

## Disclaimer

This is an unofficial, not-for-sale fan project. It is not affiliated with or endorsed by Nintendo, FromSoftware, Bandai Namco, or any other publisher or platform holder. The repository remains private during v1 development and has no open-source license.
