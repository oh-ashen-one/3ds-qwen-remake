# Building and deploying

## Toolchain

Use the official devkitPro setup and install the `3ds-dev` package group. The project expects:

- `DEVKITPRO=/opt/devkitpro`
- `DEVKITARM=/opt/devkitpro/devkitARM`
- `libctru`, `citro3d`, `citro2d`, `picasso`, `3dstools`, `tex3ds`, and `3dslink`

On this Mac, the system installation command is:

```sh
sudo dkp-pacman -S --needed 3ds-dev
```

## Validation

```sh
make validate-assets
make test-host
make
```

`make` creates a RomFS-backed `elden-ring-3ds-demake.3dsx`, plus ELF, map, list, and SMDH metadata files.

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
