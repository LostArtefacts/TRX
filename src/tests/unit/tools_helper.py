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

# update_game_strings reaches jsonschema through shared.linting, but only inside
# a validation step unrelated to anything tested here. Satisfy the import rather
# than skip the tests: a skipped test proves nothing.
for _optional in ("jsonschema", "pyjson5"):
    if importlib.util.find_spec(_optional) is None:
        sys.modules[_optional] = types.ModuleType(_optional)


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
