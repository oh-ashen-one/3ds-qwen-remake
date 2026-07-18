# Build evidence

## Packaging and telemetry candidate

- Date: 2026-07-18
- Source commit: `bc5935f7df9681db3185793f4e81cb64ebcc7a8d`
- Local source-built-toolchain `.3dsx`: `578,316` bytes; SHA-256 `2f8f7cd90329c4de2ac5e1a6a78986e1cf167d70bba490b3ca327a5898ef9ebe`.
- Deterministic SD bundle: `409,667` bytes; SHA-256 `cb53379eac49c79ef344ebabfb493c7f0d226bd298f97d392bd7b2a7db8fad23`.
- Verified install entry: `sdmc:/3ds/elden-ring-3ds-demake/elden-ring-3ds-demake.3dsx`, with the source commit and artifact hash embedded alongside it in `build-info.json`.
- Zone-budget report: 53 static props and 15 rigid bones; declared RomFS headroom is 138,003 bytes for interior, 187,866 bytes for vista, and 163,013 bytes for arena.
- Full clean local gate passed: repository audit, original-asset generation and validation, budget-report validation, deterministic host tests, native cross-build, build verification, and SD archive re-open/hash verification.

The diagnostics now display draw-call count alongside frame time, visible/culled objects, declared zone residency, measured linear-memory peak/free space, audio state, and underruns. This proves packaging structure and traceability, not a physical SD launch or hardware performance result.

## Indexed-renderer correction

- Date: 2026-07-18
- Source branch: `codex/fix-native-rendering`
- Local source-built-toolchain `.3dsx`: `578,292` bytes; SHA-256 `f1288a591d27f9ca7f3490055799dd7efb7946e041c01a4d94d68126508624e5`.
- Full local gate passed: repository audit, original-asset validation, deterministic host tests, native cross-build, metadata/RomFS/link verification, and the renderer source regression check.

The first post-M4 Azahar smoke exposed a native draw bug: the cube indices are bytes, but `C3D_DrawElements` was passed `GPU_UNSIGNED_BYTE`. That unrelated GPU enum has value `1`, which Citro3D interprets as its 16-bit index type. The renderer therefore consumed adjacent index bytes as invalid 16-bit vertex indices and produced only the clear color plus Citro2D overlays. The corrected call uses `C3D_UNSIGNED_BYTE`; `tools/verify_build.py` now rejects future changes to the wrong enum.

After rebuilding, official Azahar 2125.1.3 was restarted against the corrected artifact. The title boot rendered the vestibule geometry and player silhouette behind the translucent title panel, both-screen overlays remained present, and the emulator continued presenting frames. Computer-driven key taps remained unreliable, so this is a corrected boot/render smoke—not a gameplay, performance, or hardware acceptance claim.

## M4 content/architecture candidate

- Date: 2026-07-18
- Source commit: `4f8865195bdeeee6c093612c7500fee02ca3d38f`
- Private repository: `oh-ashen-one/elden-ring-3ds-demake`
- Draft PR: `https://github.com/oh-ashen-one/elden-ring-3ds-demake/pull/1`
- GitHub Actions PR run: `https://github.com/oh-ashen-one/elden-ring-3ds-demake/actions/runs/29657762838`
- GitHub Actions push run: `https://github.com/oh-ashen-one/elden-ring-3ds-demake/actions/runs/29657761443`
- CI result: both runs succeeded. The PR run passed repository audit, clean original-asset generation, validation, deterministic host tests, native build, 3DSX/SMDH/RomFS/link verification, report generation, and private artifact upload.
- Pinned CI toolchain: official `devkitpro/devkitarm` OCI digest `sha256:116afba8df8453961de2936ffab20dd441edf4d682856c1ec8b0e53d7ed0bbf5`; devkitARM GCC 16.1.0; `tex3ds` 2.3.0.
- CI artifact: `ashen-rift-3ds`, artifact id `8433385115`, archive size `1,199,658` bytes.
- CI `.3dsx`: `571,132` bytes; SHA-256 `e308ba55a388ed5a70b4beaefb4435323df6e6918556815f392645bea3abbf6e`.
- CI verifier also hashed the ELF (`3d0060da...`), map (`ff04094e...`), and SMDH (`b00b518f...`) and confirmed the generated registry/scene systems, rigid pose sampler, NDSP streamer, RGB565 T3X importer, model detection, indexed draws, RomFS markers, and SMDH identity.
- Local source-built-toolchain `.3dsx`: `578,300` bytes; SHA-256 `be55cb14b3508d91a69a9b5cbbb56c941dd4113b212b633ac43a8f51d6e0ca86`.
- Blender 5.2.0 LTS reopened `assets/blender/ashen-rift-source.blend` and verified 53 mesh objects, one armature, and 15 bones. Metadata fingerprints tie it to the scene descriptor and authoring script.

This candidate closes the non-hardware asset/architecture automation gap. It does not prove visual correctness, input feel, performance, memory headroom, audio continuity, sleep/wake, netloading, or SD launch on a physical console.

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
