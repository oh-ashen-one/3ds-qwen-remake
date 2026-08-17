# Shared env for ralph verify wrappers. Loop runs ON the Studio.
# Toolchain is the self-contained devkit that built the original game.
export DEVKITPRO="${DEVKITPRO:-$HOME/.local/share/elden-ring-3ds-devkit}"
export DEVKITARM="${DEVKITARM:-$DEVKITPRO/devkitARM}"
export PATH="$DEVKITARM/bin:$DEVKITPRO/tools/bin:$DEVKITPRO/bin:/opt/homebrew/bin:$PATH"
