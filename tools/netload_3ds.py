#!/usr/bin/env python3
"""Validate and run the Mac-to-3DS Homebrew Launcher netload command."""

from __future__ import annotations

import argparse
import ipaddress
import os
import shutil
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "elden-ring-3ds-demake.3dsx"
DEFAULT_LOCAL_ROOT = Path.home() / ".local" / "share" / "elden-ring-3ds-devkit"


def find_3dslink() -> Path:
    override = os.environ.get("THREEDSLINK")
    if override:
        candidate = Path(override).expanduser()
        if candidate.is_file() and os.access(candidate, os.X_OK):
            return candidate.resolve()
        raise SystemExit(f"THREEDSLINK is not an executable file: {candidate}")

    discovered = shutil.which("3dslink")
    if discovered:
        return Path(discovered).resolve()

    roots: list[Path] = []
    tools_override = os.environ.get("ASHEN_3DS_TOOLS")
    if tools_override:
        roots.append(Path(tools_override))
    project_root = Path(os.environ.get("ASHEN_3DS_ROOT", DEFAULT_LOCAL_ROOT))
    roots.append(project_root / "tools" / "bin")
    devkitpro = os.environ.get("DEVKITPRO")
    if devkitpro:
        roots.append(Path(devkitpro) / "tools" / "bin")

    for root in roots:
        candidate = root.expanduser() / "3dslink"
        if candidate.is_file() and os.access(candidate, os.X_OK):
            return candidate.resolve()
    raise SystemExit(
        "3dslink was not found; install devkitPro's 3ds-dev tools, set "
        "ASHEN_3DS_ROOT/ASHEN_3DS_TOOLS, or set THREEDSLINK directly"
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--ip", required=True, help="3DS IPv4 address shown by netloader")
    parser.add_argument("--check-only", action="store_true", help="validate without networking")
    arguments = parser.parse_args()

    try:
        address = ipaddress.ip_address(arguments.ip)
    except ValueError as error:
        raise SystemExit(f"invalid 3DS IP address: {error}") from error
    if address.version != 4:
        raise SystemExit("3dslink requires the console's IPv4 address")
    if not TARGET.is_file() or TARGET.stat().st_size == 0:
        raise SystemExit(f"missing native artifact: {TARGET.name}; run make first")

    tool = find_3dslink()
    command = [str(tool), str(TARGET), "-a", str(address)]
    if arguments.check_only:
        print(f"netload preflight passed: {tool} -> {address} ({TARGET.stat().st_size} bytes)")
        return
    print(f"netloading {TARGET.name} to {address}")
    subprocess.run(command, cwd=ROOT, check=True)


if __name__ == "__main__":
    main()
