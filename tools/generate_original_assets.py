#!/usr/bin/env python3
"""Convert the project-owned music masters to raw CTR-001 PCM streams."""

from __future__ import annotations

import wave
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SAMPLE_RATE = 22_050
TRACKS = (
    (ROOT / "assets" / "audio" / "ashen_deep_hall.wav",
     ROOT / "romfs" / "audio" / "ashen_deep_hall.pcm"),
    (ROOT / "assets" / "audio" / "ashen_gate.wav",
     ROOT / "romfs" / "audio" / "ashen_gate.pcm"),
    (ROOT / "assets" / "audio" / "valley_after_dawn.wav",
     ROOT / "romfs" / "audio" / "valley_after_dawn.pcm"),
)


def convert(master: Path, output: Path) -> int:
    with wave.open(str(master), "rb") as source:
        if source.getnchannels() != 1:
            raise SystemExit(f"{master.name} must be mono")
        if source.getsampwidth() != 2:
            raise SystemExit(f"{master.name} must be PCM16")
        if source.getframerate() != SAMPLE_RATE:
            raise SystemExit(f"{master.name} must be {SAMPLE_RATE} Hz")
        payload = source.readframes(source.getnframes())
    output.parent.mkdir(parents=True, exist_ok=True)
    if not output.exists() or output.read_bytes() != payload:
        output.write_bytes(payload)
    return len(payload)


def main() -> None:
    for master, output in TRACKS:
        size = convert(master, output)
        print(f"generated {output.relative_to(ROOT)} ({size} bytes)")


if __name__ == "__main__":
    main()
