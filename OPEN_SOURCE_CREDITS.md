# Open-source credits and build stack

Ashen Rift is original game code and content built for the original Nintendo
3DS. It is not a ROM hack, a commercial-game port, or a project built with
Nintendo's proprietary SDK. The project relies on community-maintained
homebrew software to enable the console, compile the source, render both
screens, package the application, and test native builds.

Third-party projects retain their own copyrights and licenses. Links below go
to the upstream project or its official source repository. The console tools
are not redistributed by this repository.

## From a stock 3DS to homebrew

| Project | Role in the workflow |
| --- | --- |
| [3DS Hacks Guide](https://github.com/hacks-guide/Guide_3DS) | The maintained, model- and firmware-specific process followed to modify the Japanese original 3DS safely. |
| [MSET9 by zoogie](https://github.com/zoogie/MSET9) | Temporary System Settings exploit used on the original 3DS. It prepared a crafted SD profile, launched the installer, and was removed after setup. |
| [SafeB9SInstaller by d0k3](https://github.com/d0k3/SafeB9SInstaller) | Installer launched by MSET9 to install boot9strap after its cryptographic checks passed. |
| [boot9strap by SciresM](https://github.com/SciresM/boot9strap) | Persistent early-boot entry point that allows the console to load a custom `boot.firm`. |
| [Luma3DS by LumaTeam](https://github.com/LumaTeam/Luma3DS) | Custom firmware loaded as `boot.firm`; enables the console environment in which standard homebrew can run. The recorded console setup used Luma3DS 13.4. |
| [3DS Hacks Guide Finalize](https://github.com/hacks-guide/finalize) | Finalization scripts used for DSP setup, homebrew installation, SD cleanup, and system-file backups. |
| [GodMode9 by d0k3](https://github.com/d0k3/GodMode9) | File, NAND, and recovery utility used to create the essential-files and SysNAND backups during finalization. |
| [3DS Homebrew Menu by devkitPro](https://github.com/devkitPro/3ds-hbmenu) | Lists and launches `.3dsx` applications from the SD card, including Ashen Rift. |
| [Homebrew Launcher Loader by PabloMK7](https://github.com/PabloMK7/homebrew_launcher_dummy) | HOME Menu entry installed by the finalization process to open the Homebrew Menu. |

The relevant launch chain is:

```text
MSET9 -> SafeB9SInstaller -> boot9strap -> Luma3DS
      -> Homebrew Menu -> Ashen Rift.3dsx
```

MSET9 and SafeB9SInstaller were installation-stage tools. They are not part of
the running game. Luma3DS and the Homebrew Menu provide the environment that
launches the final `.3dsx`.

## Standard finalization applications

The official finalization process also installed the following open-source
utilities. They are useful parts of the console's homebrew setup, but they are
not Ashen Rift runtime dependencies.

| Project | Purpose |
| --- | --- |
| [FBI-NH](https://github.com/nh-server/FBI-NH) | Installs and manages CIA-format applications. |
| [Anemone3DS](https://github.com/astronautlevel2/Anemone3DS) | Installs themes, splashes, and badges. |
| [Checkpoint](https://github.com/BernardoGiordano/Checkpoint) | Backs up and restores game save data. |
| [ftpd](https://github.com/mtheall/ftpd) | Provides wireless FTP access to the 3DS SD card. |
| [Universal-Updater](https://github.com/Universal-Team/Universal-Updater) | Downloads and updates homebrew applications over Wi-Fi. |

## Native Nintendo 3DS development stack

| Project | Role in Ashen Rift |
| --- | --- |
| [devkitPro](https://github.com/devkitPro) and its [build scripts](https://github.com/devkitPro/buildscripts) | Provides the maintained open homebrew toolchain and `3ds-dev` package group. |
| [devkitARM](https://github.com/devkitPro/buildscripts) | ARM cross-compiler toolchain used to turn the C++17 source into code for the original 3DS ARM processors. |
| [libctru](https://github.com/devkitPro/libctru) | Nintendo 3DS user-mode APIs for input, audio, files, services, memory, and application lifecycle. |
| [citro3d](https://github.com/devkitPro/citro3d) | PICA200 GPU wrapper used for the low-poly environments, characters, bosses, fog, shadows, and indexed 3D rendering. |
| [citro2d](https://github.com/devkitPro/citro2d) | PICA200-backed 2D library used for HUD elements, text, health bars, overlays, and the lower-screen interface. |
| [Picasso](https://github.com/devkitPro/picasso) | Assembles the PICA200 vertex shader used by the renderer. |
| [tex3ds](https://github.com/devkitPro/tex3ds) | Converts the original environment texture atlas into the 3DS-native T3X format. |
| [3dstools](https://github.com/devkitPro/3dstools) | Supplies native metadata and packaging utilities used by the 3DS build pipeline. |
| [3dslink](https://github.com/devkitPro/3dslink) | Supports Wi-Fi netloading during development. The persistent release is installed through the verified SD-card bundle. |

The CI evidence records a pinned official `devkitpro/devkitarm` container,
devkitARM GCC 16.1.0, and tex3ds 2.3.0. See
[BUILD_EVIDENCE.md](docs/BUILD_EVIDENCE.md) for the exact build snapshot.

## Content, validation, and collaboration tools

| Project or service | Role in the production workflow |
| --- | --- |
| [Blender](https://github.com/blender/blender) | Editable low-poly scene, original box-primitive models, environment layout, and 15-bone rigid hierarchy. |
| [Python](https://github.com/python/cpython) | MSET9's macOS script plus the project's asset generation, scene conversion, validation, privacy-audit, and packaging scripts. |
| [Azahar](https://github.com/azahar-emu/azahar) | Open-source 3DS emulator used for native boot/render smoke tests and visual debugging before physical-console runs. The recorded preview used Azahar 2125.1.3. |
| [Git](https://github.com/git/git) | Source history and local change management. |
| [GitHub Actions runner](https://github.com/actions/runner) | CI execution behind the repository's build, tests, policy audit, native verification, and artifact generation. |
| [devkitPro devkitARM container](https://hub.docker.com/r/devkitpro/devkitarm) | Pinned, reproducible CI environment used by GitHub Actions. |

## What belongs to Ashen Rift

The following were created for this repository rather than extracted from a
commercial title:

- C++ gameplay, rendering, streaming, audio, input, lifecycle, and boss logic
- six-zone world layout and memory budgets
- player, NPC, horses, wildlife, Ashen Warden, Gore-Tusk, and Arashi designs
- low-poly geometry, texture atlas, rigid animations, dialogue, maps, and UI
- deterministic tests, repository privacy audit, build verification, and SD
  packaging scripts

The source and original project assets are distributed under the repository's
[MIT License](LICENSE). No commercial game code, models, textures, music,
dialogue, console keys, NAND images, or user-unique SD data are included.

## Other credited production services

The three original music tracks were supplied by the project owner after being
created with [Suno](https://suno.com/). Suno is a production service, not an
open-source runtime dependency. OpenAI Codex assisted with implementation,
testing, documentation, and iteration; it is likewise not bundled into or
required by the game.

## Public project repository

Ashen Rift's source, documentation, tests, and original asset pipeline are
published at:

<https://github.com/oh-ashen-one/elden-ring-3ds-demake>
