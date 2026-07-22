"""Shared plumbing for the tools/ unit tests.

The scripts under tools/ have no .py extension, so they cannot simply be
imported. This loads one by path, and satisfies the third-party imports they
reach for transitively but do not need for anything under test.
"""

from __future__ import annotations

import importlib.machinery
import importlib.util
import sys
import types
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools"))

# update_game_strings parses JSON5 through shared.json_utils, which imports
# pyjson5 - unrelated to anything tested here. Satisfy the import rather than
# skip the tests: a skipped test proves nothing.
if importlib.util.find_spec("pyjson5") is None:
    sys.modules["pyjson5"] = types.ModuleType("pyjson5")


def load(name: str):
    """Import a tools/ script by name."""
    spec = importlib.util.spec_from_loader(
        name,
        importlib.machinery.SourceFileLoader(name, str(ROOT / "tools" / name)),
    )
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module
