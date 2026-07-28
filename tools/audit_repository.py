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
    "essential.exefs",
    "localfriendcodeseed_b",
    "movable.sed",
    "nand.bin",
    "otp.bin",
    "private",
    "secureinfo_a",
    "seeddb.bin",
}
FORBIDDEN_SUFFIXES = {".3ds", ".cia", ".key", ".pem", ".sav"}
FORBIDDEN_PARTS = {"nintendo 3ds", "sd backup", "sd-card-backup", "credentials"}
SECRET_PATTERNS = (
    re.compile(rb"-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----"),
    re.compile(rb"\bgh[opusr]_[A-Za-z0-9]{30,}\b"),
    re.compile(rb"\bgithub_pat_[A-Za-z0-9_]{30,}\b"),
    re.compile(rb"\bAKIA[A-Z0-9]{16}\b"),
)
PERSONAL_PATTERNS = (
    re.compile(rb"/" + b"Users" + rb"/[^/\s]+/"),
    re.compile(rb"/" + b"Volumes" + rb"/[^/\r\n]+"),
    re.compile(rb"[A-Za-z]:\\\\" + b"Users" + rb"\\\\[^\\\s]+", re.IGNORECASE),
    re.compile(rb"\bmi" + b"dir" + rb"\b", re.IGNORECASE),
    re.compile(rb"\bhaar" + b"itth" + rb"@gmail\.com\b", re.IGNORECASE),
)


def fail(message: str) -> None:
    print(f"repository audit failed: {message}", file=sys.stderr)
    raise SystemExit(1)


def git_output(arguments: list[str]) -> bytes:
    result = subprocess.run(
        ["git", "-c", f"safe.directory={ROOT}", *arguments],
        cwd=ROOT,
        check=False,
        capture_output=True,
    )
    if result.returncode != 0:
        detail = result.stderr.decode("utf-8", errors="replace").strip()
        fail(f"git {' '.join(arguments)} failed: {detail or 'unknown git error'}")
    return result.stdout


def release_history_roots() -> list[str]:
    """Exclude only GitHub's ephemeral PR merge while auditing both real parents."""
    parents = git_output(["show", "-s", "--format=%P", "HEAD"]).decode("ascii").split()
    if len(parents) != 2:
        return ["HEAD"]
    metadata = git_output(["show", "-s", "--format=%ce%x00%s", "HEAD"]).split(b"\x00", 1)
    if len(metadata) != 2:
        return ["HEAD"]
    committer_email, subject = metadata
    synthetic_subject = re.fullmatch(
        rb"Merge [0-9a-f]{40} into [0-9a-f]{40}", subject.strip()
    )
    if committer_email.strip().endswith(b"@github.com") and synthetic_subject:
        return parents
    return ["HEAD"]


def validate_release_history() -> None:
    history_roots = release_history_roots()
    identities = git_output(["log", "--format=%ae%x00%ce", *history_roots])
    allowed_suffixes = (b"@users.noreply.github.com", b"@github.com")
    emails = {value.strip() for value in identities.split(b"\x00") if value.strip()}
    disallowed = sorted(
        value.decode("utf-8", errors="replace")
        for value in emails
        if not value.endswith(allowed_suffixes)
    )
    if disallowed:
        fail(f"release history contains non-noreply identities: {disallowed}")

    patch_history = git_output(["log", "-p", "--no-ext-diff", *history_roots, "--", "."])
    if any(pattern.search(patch_history) for pattern in SECRET_PATTERNS + PERSONAL_PATTERNS):
        fail("release history contains a secret or personal path pattern")


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
                if any(pattern.search(payload) for pattern in SECRET_PATTERNS + PERSONAL_PATTERNS):
                    violations.append(path.as_posix())
    if violations:
        fail(f"forbidden tracked files: {sorted(violations)}")
    license_path = ROOT / "LICENSE"
    if not license_path.is_file() or not license_path.read_text(encoding="utf-8").startswith(
        "MIT License\n"
    ):
        fail("public releases require the repository MIT LICENSE")
    validate_release_history()
    print(f"repository audit passed: {len(tracked)} repository files, no forbidden payloads")


if __name__ == "__main__":
    main()
