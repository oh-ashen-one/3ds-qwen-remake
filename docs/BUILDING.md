# Building and deploying

## Toolchain

Use the official devkitPro setup and install the `3ds-dev` package group. The project expects:

- `DEVKITPRO=/opt/devkitpro`
- `DEVKITARM=/opt/devkitpro/devkitARM`
- `libctru`, `citro3d`, `citro2d`, `picasso`, `3dstools`, `tex3ds`, and `3dslink`

CI pins the official `devkitpro/devkitarm` image by OCI digest and records the compiler and `tex3ds` versions in its private artifact report. Updating that digest is an intentional toolchain change and requires a clean local/CI gate.

If the official 3DS libraries and host tools were installed in a user-writable prefix instead of the system devkitPro prefix, set `ASHEN_3DS_ROOT` to that prefix, or set `CTRULIB` and `ASHEN_3DS_TOOLS` separately. The local bootstrap used for this project is detected automatically at `~/.local/share/elden-ring-3ds-devkit`.

On this Mac, the system installation command is:

```sh
sudo dkp-pacman -S --needed 3ds-dev
```

## Validation

```sh
make audit-repo
make validate-assets
make test-host
make verify-build
make package-sd
```

`make verify-build` performs the native build and validates the 3DSX header, SMDH metadata, embedded RomFS markers, linked subsystems, and artifact hashes. The build creates `elden-ring-3ds-demake.3dsx`, plus ELF, map, list, SMDH, and JSON validation files.

The asset phase runs before compilation. It generates the asset registry, expands the authored scene into host-test arrays plus three compact RomFS zone blobs, synthesizes original PCM audio, and invokes `tex3ds --atlas` for the original RGB565 environment texture. On hardware, `ZoneResources` allocates the requested blob on preload and frees the prior zone after entry; the static arrays are excluded from the native build and retained only for host validation. Generated files are ignored and recreated by `make clean && make validate-assets`.

## Blender source workflow

The editable `.blend` is generated from the same declarative source consumed by the handheld converter:

```sh
blender --background --python assets/blender/author_lowpoly.py -- \
  --output assets/blender/ashen-rift-source.blend
make validate-assets
```

Edit the source descriptor and authoring script together; do not hand-edit generated C++ headers or T3X data. The repository retains the Blender source but not generated build outputs.

## Wi-Fi development loop

1. Start Homebrew Launcher on the 3DS.
2. Press `Y` to activate netloader and note the console's IP address.
3. Keep the Mac and 3DS on the same network.
4. Run `make check-netload IP=<3DS-IP>` to preflight without networking.
5. Run `make run IP=<3DS-IP>` to transfer and launch.

The preflight validates the address, build artifact, and discoverable official `3dslink` executable. `make run` performs the same checks before the transient launch. A successful transfer is not proof that the complete game works.

## Persistent SD installation

Build the deterministic bundle after native verification:

```sh
make package-sd
```

The command verifies the `.3dsx` against `build-validation.json`, writes the exact source commit and SHA-256 into `build-info.json`, and creates:

```text
dist/ashen-rift-sd-bundle.zip
dist/sdmc/3ds/elden-ring-3ds-demake/elden-ring-3ds-demake.3dsx
dist/sdmc/3ds/elden-ring-3ds-demake/build-info.json
dist/sd-bundle-validation.json
```

Extract the ZIP at the SD-card root, or copy the contents of `dist/sdmc/` to the root. The resulting application path must be:

```text
sdmc:/3ds/elden-ring-3ds-demake/elden-ring-3ds-demake.3dsx
```

Launch it from Homebrew Launcher with the Mac disconnected. Do not hot-swap the SD card while Homebrew Launcher is running. Use a safe eject or a trusted FTP homebrew workflow.

Record the exact commit and SHA-256 from the generated metadata before either deployment path. Follow [HARDWARE_TEST.md](HARDWARE_TEST.md); a transfer alone is not a pass.
