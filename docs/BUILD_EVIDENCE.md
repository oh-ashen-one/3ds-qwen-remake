# Build evidence

## Milestone M1–M3 candidate

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

This proves that the source, tests, asset policy, shader, RomFS, and native link pipeline work in two build environments. It does **not** prove that rendering, input, audio, performance, netloading, sleep/wake, or the full gameplay loop work on a physical New Nintendo 3DS. Those claims remain pending in `HARDWARE_TEST.md`.
