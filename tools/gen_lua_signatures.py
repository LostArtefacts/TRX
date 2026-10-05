#!/usr/bin/env python3
"""Write trx.internal.signatures, the strict-mode signatures of the annotated modules.

The engine build embeds the same module through embed_trx_lua.py; this writes
it as a file for the unit tests, which read modules from disk.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from shared.luaannot import signatures_module  # noqa: E402


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--module-root", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    args.output.write_text(signatures_module(args.module_root), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
