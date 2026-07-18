#!/usr/bin/env python3
"""Generate deterministic, project-original runtime assets."""

from __future__ import annotations

import math
import struct
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "romfs" / "audio" / "ambient.pcm"
SAMPLE_RATE = 22_050
DURATION_SECONDS = 8


def synthesize_ambient() -> bytes:
    frequencies = (110.0, 146.83, 164.81)
    frames = bytearray()
    total_samples = SAMPLE_RATE * DURATION_SECONDS
    for index in range(total_samples):
        time = index / SAMPLE_RATE
        loop_phase = index / total_samples
        fade = math.sin(loop_phase * math.tau) ** 2
        drone = sum(
            math.sin(math.tau * frequency * time + voice * 0.7) / (voice + 2.0)
            for voice, frequency in enumerate(frequencies)
        )
        shimmer = math.sin(math.tau * 0.125 * time) * math.sin(math.tau * 293.66 * time) * 0.05
        sample = max(-1.0, min(1.0, drone * (0.25 + fade * 0.08) + shimmer))
        frames.extend(struct.pack("<h", int(sample * 11_000)))
    return bytes(frames)


def main() -> None:
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    payload = synthesize_ambient()
    if not OUTPUT.exists() or OUTPUT.read_bytes() != payload:
        OUTPUT.write_bytes(payload)
    print(f"generated {OUTPUT.relative_to(ROOT)} ({len(payload)} bytes)")


if __name__ == "__main__":
    main()
