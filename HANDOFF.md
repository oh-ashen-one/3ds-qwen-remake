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
- The Reach is now a Japanese-alpine valley with a winding Azusa-inspired
  river, a Hikari snowmelt tributary, bridge, ford, broad valley floor, raised
  meadow terraces, cedar groves,
  pink/blue/gold flower families, a mountain-pass torii, stepped snowcapped
  ranges, a visible sun, and three drifting cloud banks.
- The valley contains procedurally moving deer, foxes, and river cranes. Three
  differently colored horses are independently mountable and retain saddle,
  tail, ear, leg-gait, rider-placement, stamina-gallop, mounted-heal, dismount,
  and recall behavior. The lower screen maps the river, every horse, the active
  horse, the pass, objective text, and RIDING status.
- The fourth zone has its own RomFS scene blob, manifest, generated asset
  registry membership, Blender source objects, streaming/budget validation, and
  host flow tests. Exploration music continues through the field.

Primary files:

- `assets/scene_source.json`
- `source/core.cpp`
- `source/renderer.cpp`
- `include/demake/core.hpp`
- `include/demake/renderer.hpp`
- `tests/core_tests.cpp`
- `tests/original_3ds_contract_tests.py`

## Budget

The game continues to reuse one indexed cube VBO. The expanded Sunlit Reach
declares a 204-draw budget and 768 KiB runtime budget. Its 384 generated static
boxes are coarse-grid, view, sightline, and distance culled; horses and wildlife
are fixed box assemblies with whole-creature range rejection. Re-run the
validators whenever the scene source changes.

## Verified Locally

Run the following before handing the branch onward:

```text
make test-host
make validate-assets
make verify-build
make package-sd
make audit-repo
```

Official Azahar 2125.1.3 rendered the expanded unmounted valley at 60
application FPS during development (roughly 2.6–2.8 ms reported emulator frame
time in the initial river view). Multi-horse behavior is host-tested; a full
latest-build mounted traversal still needs emulator and physical capture. This
is only local evidence. The latest artifact has not been copied to and replayed
on the physical 3DS, so hardware acceptance remains pending.

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
3. Ride all three horses, gallop, heal while mounted, dismount, recall, traverse
   the river/bridge/ford, reach every field edge, and inspect wildlife,
   cloud/mountain silhouettes, flowers, and cedar pop-in while rotating.
4. Record diagnostics at the field's busiest view. Target 30 FPS with a 24 FPS
   floor, then update the hardware report with measured—not emulator—results.
5. Copy the freshly verified SD bundle only with the 3DS fully powered off and
   verify its hash before ejecting the card.
