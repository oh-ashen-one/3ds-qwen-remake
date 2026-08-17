#!/usr/bin/env python3
"""Standalone marker-strip pass (verify prelude): truncate any source file that
still contains leaked Ralph ### FILE / END FILE lines."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from qwen_iteration import repo_root, strip_leaked_markers  # noqa: E402

if __name__ == "__main__":
    strip_leaked_markers(repo_root())
