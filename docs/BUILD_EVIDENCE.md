# Build evidence

## Runtime-hardening candidate

- Date: 2026-07-18
- Source commit: `89a520244d4b86071ee62b222d995b38a074ab14`
- Private repository: `oh-ashen-one/elden-ring-3ds-demake`
- GitHub Actions run: `https://github.com/oh-ashen-one/elden-ring-3ds-demake/actions/runs/29656291832`
- CI result: success; original-asset validation, deterministic host tests, native 3DSX build, report generation, and artifact upload all passed.
- CI artifact: `ashen-rift-3ds`, artifact id `8432968668`, archive size `1,079,490` bytes.
- CI `.3dsx`: `557,768` bytes; SHA-256 `2a9b7b801661e48e810d4d9bb03256f826b91ea53dbbe0f0f687124c684cb4d4`.
- Local source-built-toolchain `.3dsx`: `566,488` bytes; SHA-256 `89877e0491e73cd1337ebaefbcfe06f0baa661450483efbcc2c735eea249bafe`.
- `3dsxdump` accepted the local artifact and reported 44 code pages, 3 read-only-data pages, 2 data pages, and 7 BSS pages.
- New host coverage verifies wraparound D-pad quick-item selection alongside the existing movement, stamina, zone, combat, dodge, heal, death, victory, and restart checks.

### Emulator smoke

The title-enabled local artifact was loaded in official Azahar 2125.1.3 for macOS ARM64. The emulator rendered the native 3D vestibule, expanded rigid-part player model, title overlay, top-screen text, and bottom-screen zone/controls/quick-item/flask UI. Azahar's status display reported 60 application FPS during this boot smoke.

This smoke found a real integration defect: raw citro3d rendering replaced citro2d's shader and vertex-buffer state, so UI batches were initially invisible. Commit `89a5202` gives the 3D renderer its own persistent attribute/buffer descriptors and explicitly restores citro2d state before overlay rendering. A rebuilt artifact displayed both screens correctly after that fix.

Computer-driven key taps were not reliable enough to claim an end-to-end emulator playthrough. Emulator FPS is also not physical-console performance evidence. Full input, audio, transition, combat, sleep/wake, and restart acceptance therefore remains in `HARDWARE_TEST.md`.

## Prior M1–M3 candidate

- Date: 2026-07-18
- Source commit: `0ad06b66144e85ac2fc0be31e227e66ca7705fa4`
- Private repository: `oh-ashen-one/elden-ring-3ds-demake`
- GitHub Actions run: `https://github.com/oh-ashen-one/elden-ring-3ds-demake/actions/runs/29655685968`
- CI result: success; original-asset generation/validation, deterministic host tests, native 3DSX build, report generation, and artifact upload all passed.
- CI artifact: `ashen-rift-3ds`, artifact id `8432791976`, archive size `1,074,719` bytes.
- CI `.3dsx`: `556,316` bytes; SHA-256 `a90871d5498efd83c6000a02bcc6fcea63b0668f3dbc4978d316a8c3da3052d5`.
- Local source-built-toolchain `.3dsx`: `565,008` bytes; SHA-256 `6493caaec16cb38755e6c40b5c58ba6cab1dbb92ffb949408afe8ea7e3ab38e0`.
- `3dsxdump` accepted the local artifact and reported 44 code pages, 3 read-only-data pages, 2 data pages, and 7 BSS pages.
- Embedded RomFS inspection found `audio/ambient.pcm` and the original Veiled Keeper dialogue.

The local and CI hashes differ because they were linked against different official devkitPro package snapshots. Reproducibility is defined within the pinned CI container environment; the CI artifact is the handoff candidate until the next source change.

## Evidence boundary

This proves that the source, tests, asset policy, shader, RomFS, native link pipeline, and title-screen emulator smoke work. It does **not** prove that input, audio, performance, netloading, sleep/wake, or the full gameplay loop work on a physical New Nintendo 3DS. Those claims remain pending in `HARDWARE_TEST.md`.
