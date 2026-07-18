# Building and deploying

## Toolchain

Use the official devkitPro setup and install the `3ds-dev` package group. The project expects:

- `DEVKITPRO=/opt/devkitpro`
- `DEVKITARM=/opt/devkitpro/devkitARM`
- `libctru`, `citro3d`, `citro2d`, `picasso`, `3dstools`, `tex3ds`, and `3dslink`

CI pins the official `devkitpro/devkitarm` image by OCI digest and records the compiler and `tex3ds` versions in its private artifact report. Updating that digest is an intentional toolchain change and requires a clean local/CI gate.

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
```

`make verify-build` performs the native build and validates the 3DSX header, SMDH metadata, embedded RomFS markers, linked subsystems, and artifact hashes. The build creates `elden-ring-3ds-demake.3dsx`, plus ELF, map, list, SMDH, and JSON validation files.

The asset phase runs before compilation. It generates the asset registry, expands the authored scene into compact fixed-size arrays, synthesizes original PCM audio, and invokes `tex3ds --atlas` for the original RGB565 environment texture. Generated files are ignored and recreated by `make clean && make validate-assets`.

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
4. Run `make run IP=<3DS-IP>`.

This is a transient launch for rapid iteration. A successful transfer is not proof that the complete game works.

## Persistent SD installation

Copy the application to:

```text
sdmc:/3ds/elden-ring-3ds-demake/elden-ring-3ds-demake.3dsx
```

Launch it from Homebrew Launcher with the Mac disconnected. Do not hot-swap the SD card while Homebrew Launcher is running. Use a safe eject or a trusted FTP homebrew workflow.

Record the exact commit and SHA-256 before either deployment path. Follow [HARDWARE_TEST.md](HARDWARE_TEST.md); a transfer alone is not a pass.
