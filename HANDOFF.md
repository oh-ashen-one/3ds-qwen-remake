# Laptop Handoff: Box-Model Character Redesign

## Checkout

Repository: `oh-ashen-one/elden-ring-3ds-demake`

Branch: `agent/character-redesign-handoff`

```sh
git fetch origin
git switch --track origin/agent/character-redesign-handoff
```

The generated `.3dsx`, build directory, SD bundle, emulator, and screenshots are
intentionally not committed. Rebuild them from source on the laptop.

## Implemented

- The player now has a distinct original-3DS box model assembled around the
  existing 15-bone rigid pose: dark iron body, large pauldrons, short mantle,
  crimson chest/skirt tabard, barred slit helmet, and a three-part short sword.
- The Ashen Warden now uses dark iron instead of the old red body, with oversized
  shoulders, a bright cross-shaped ember visor, chest plate, asymmetric hanging
  armor/cape panels, and a three-part gate-blade.
- The dead Warden is now a five-piece collapsed armor-and-sword composition
  instead of one flat red slab.
- Gameplay, hitboxes, animation sampling, controls, combat timing, music, zones,
  and the Veiled Keeper model were not changed.
- The renderer contract now requires the dedicated player assembly and Warden
  identity markers.

Primary files:

- `source/renderer.cpp`
- `include/demake/renderer.hpp`
- `tests/original_3ds_contract_tests.py`

## Budget

The redesign remains cube-only and reuses the existing indexed cube VBO:

- Player: approximately 28 box draws, including the blob shadow.
- Living Warden: approximately 29 box draws, including the blob shadow.
- Estimated worst arena frame: approximately 73 draws.
- Declared arena budget: 84 draws.

No new texture, model, audio, RomFS, or runtime allocation was added.

## Verified Locally

The following passed before handoff:

```text
make test-host
make validate-assets
make verify-build
make audit-repo
```

Verified native artifact:

```text
size:   25,636,268 bytes
sha256: 86151392cf0d17a8f8124808c17e1942bbe02248fdf9980161fef70b94a5dd59
```

Official Azahar 2125.1.3 booted that artifact and rendered the redesigned player
in the Sable Expanse at 60 application FPS. The player, mantle, red tabard, helmet,
and sword were visible, although the current night palette is very dark. The
Warden redesign has compiled but has not yet received an emulator screenshot.

The latest artifact has not been copied to the physical SD card. Build or
emulator evidence is not a physical-console acceptance result.

## Rebuild

Set the devkitPro environment as appropriate for the laptop. A typical macOS
installation uses:

```sh
export DEVKITPRO=/opt/devkitpro
export DEVKITARM="$DEVKITPRO/devkitARM"
export PATH="$DEVKITPRO/tools/bin:$DEVKITARM/bin:$PATH"

make test-host
make validate-assets
make verify-build
make package-sd
```

Run the native build in Azahar without installing it:

```sh
Azahar.app/Contents/MacOS/azahar --windowed ./elden-ring-3ds-demake.3dsx
```

## Next Checks

1. Capture clear front/back player views and a living Warden view in Azahar.
2. If required, brighten only the character palette constants in
   `Renderer::renderPlayer` and `Renderer::renderBoss`; do not add textures or a
   new mesh pipeline for this pass.
3. Re-run the four validation commands above.
4. With the 3DS fully powered off, copy the rebuilt application to the SD card
   and verify its hash before ejecting.
5. On the physical original 3DS, confirm player/visor readability, attack and
   dodge poses, Warden telegraphs, death composition, draw count, frame time,
   and audio continuity. Target 30 FPS with a 24 FPS floor.
