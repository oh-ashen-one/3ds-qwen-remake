# Laptop Handoff: Sunlit Reach Quality Upgrade

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

- The redesigned player and Ashen Warden remain cube-based, original-3DS-safe
  models with distinct silhouettes, armor layers, weapons, and rigid animation.
- The whole game uses brighter clear colors, stone, sky, ground, character, and
  UI palettes so shapes remain readable on the dimmer CTR-001 display.
- The Warden has 140 health, slower and clearer attack telegraphs, lower damage,
  smaller hit radii, gentler pursuit, and longer recovery. Light/heavy player
  attacks now deal 20/38 damage, and the dodge travels farther.
- Victory no longer immediately ends the slice. Pressing A begins a golden,
  preload-before-unload transition into a fourth streamed zone: **The Sunlit
  Reach**.
- The Reach contains a broad grassland, pale road, meadow rises, deterministic
  grass scatter, layered mountains and snowcaps, clouds, a visible sun, and a
  distant waystone. Its camera and draw distance were widened for the vista.
- The horse is a fully rendered low-poly figure with saddle, tail, ears, animated
  legs, rider placement, mounting, dismounting, stamina-limited galloping, and
  recall. The lower screen exposes contextual RIDE/DISMOUNT and CALL actions,
  a field-scale map, horse/waystone markers, objective text, and RIDING status.
- The fourth zone has its own RomFS scene blob, manifest, generated asset
  registry membership, Blender source objects, streaming/budget validation, and
  host flow tests. Exploration music continues through the field.

Primary files:

- `source/renderer.cpp`
- `include/demake/renderer.hpp`
- `tests/original_3ds_contract_tests.py`

## Budget

The game continues to reuse one indexed cube VBO. The Sunlit Reach declares a
128-draw budget and 512 KiB runtime budget. Its 71 generated static boxes are
coarse-grid, view, and distance culled; the horse and rider are small fixed box
assemblies. Re-run the validators whenever the scene source changes.

## Verified Locally

Run the following before handing the branch onward:

```text
make test-host
make validate-assets
make verify-build
make package-sd
make audit-repo
```

Official Azahar 2125.1.3 rendered both the unmounted and mounted Reach at 60
application FPS during development (roughly 0.9–1.0 ms reported emulator frame
time). This is only emulator evidence. The latest artifact has not been copied
to and replayed on the physical 3DS, so hardware acceptance remains pending.

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

1. On the physical original 3DS, play from the vestibule through the Warden and
   into the Reach; confirm that brighter colors do not wash out and text remains
   readable.
2. Confirm all Warden telegraphs, damage, recovery openings, death transition,
   and music changes. Tune only after observing the real console.
3. Ride, gallop, heal while mounted, dismount, recall the horse, reach every
   field edge, and inspect mountain/grass pop-in while rotating the camera.
4. Record diagnostics at the field's busiest view. Target 30 FPS with a 24 FPS
   floor, then update the hardware report with measured—not emulator—results.
5. Copy the freshly verified SD bundle only with the 3DS fully powered off and
   verify its hash before ejecting the card.
