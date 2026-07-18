#!/usr/bin/env python3
"""Reject tracked secrets, console-unique files, backups, and release packages."""

from __future__ import annotations

import subprocess
import sys
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
FORBIDDEN_BASENAMES = {
    ".env",
    "aes_keys.txt",
    "boot9.bin",
    "boot11.bin",
    "movable.sed",
    "otp.bin",
    "seeddb.bin",
}
FORBIDDEN_SUFFIXES = {".3ds", ".cia", ".key", ".pem", ".sav"}
FORBIDDEN_PARTS = {"nintendo 3ds", "sd backup", "sd-card-backup", "credentials"}
SECRET_PATTERNS = (
    re.compile(rb"-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----"),
    re.compile(rb"\bghp_[A-Za-z0-9]{30,}\b"),
    re.compile(rb"\bgithub_pat_[A-Za-z0-9_]{30,}\b"),
    re.compile(rb"\bAKIA[A-Z0-9]{16}\b"),
)


def fail(message: str) -> None:
    print(f"repository audit failed: {message}", file=sys.stderr)
    raise SystemExit(1)


def main() -> None:
    result = subprocess.run(
        [
            "git",
            "-c",
            f"safe.directory={ROOT}",
            "ls-files",
            "-z",
            "--cached",
            "--others",
            "--exclude-standard",
        ],
        cwd=ROOT,
        check=False,
        capture_output=True,
    )
    if result.returncode != 0:
        detail = result.stderr.decode("utf-8", errors="replace").strip()
        fail(f"git file inventory failed: {detail or 'unknown git error'}")
    tracked = [Path(raw.decode("utf-8")) for raw in result.stdout.split(b"\0") if raw]
    violations: list[str] = []
    for path in tracked:
        lowered = path.as_posix().lower()
        if path.name.lower() in FORBIDDEN_BASENAMES or path.name.lower().startswith(".env."):
            violations.append(path.as_posix())
        elif path.suffix.lower() in FORBIDDEN_SUFFIXES:
            violations.append(path.as_posix())
        elif any(part in lowered for part in FORBIDDEN_PARTS):
            violations.append(path.as_posix())
        else:
            absolute = ROOT / path
            if absolute.is_file() and absolute.stat().st_size <= 2 * 1024 * 1024:
                payload = absolute.read_bytes()
                if any(pattern.search(payload) for pattern in SECRET_PATTERNS):
                    violations.append(path.as_posix())
    if violations:
        fail(f"forbidden tracked files: {sorted(violations)}")
    if any(path.name.lower().startswith("license") for path in tracked):
        fail("v1 must remain private without an open-source license")
    print(f"repository audit passed: {len(tracked)} repository files, no forbidden payloads")


if __name__ == "__main__":
    main()
