"""Expand engine .def X-macro files with the real C preprocessor.

The engine's .def files may include other .def files (e.g. pickups.def), so
scraping them with regexes misses generated entries. Instead, run `cpp` with
caller-supplied macro definitions that emit greppable rows, exactly as the
engine compiles them.
"""

from __future__ import annotations

import functools
import re
import shutil
import subprocess
import tempfile
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1]
SRC_DIR = TOOLS_DIR.parent / "src"

RE_ADJACENT_STRINGS = re.compile(r'"\s+"')


def expand_def_file(def_path: Path, macro_defs: dict[str, str]) -> str:
    """Preprocess a .def file with the given macro definitions.

    macro_defs maps macro signatures to bodies, e.g.
    {"X_CATALOG_ID(x)": "SYMBOL(x)"}. Returns the preprocessed text with
    adjacent string literals merged ('"key_" "5"' -> '"key_5"').
    Results are cached for the lifetime of the process.
    """
    return _expand_def_file(
        def_path.resolve(), tuple(sorted(macro_defs.items()))
    )


@functools.cache
def _expand_def_file(
    def_path: Path, macro_defs: tuple[tuple[str, str], ...]
) -> str:
    cpp = shutil.which("cpp")
    if cpp is None:
        raise RuntimeError("cpp (the C preprocessor) is required but missing")

    stub = ""
    for signature, body in macro_defs:
        # A body that spans lines keeps its continuations. Without them the
        # first newline ends the #define and the rest becomes stray text.
        body = body.replace("\n", " \\\n")
        stub += f"#define {signature} {body}\n"
    stub += f'#include "{def_path}"\n'

    with tempfile.NamedTemporaryFile("w", suffix=".c") as tmp:
        tmp.write(stub)
        tmp.flush()
        result = subprocess.run(
            [cpp, "-P", "-I", str(SRC_DIR), tmp.name],
            capture_output=True,
            text=True,
        )
    if result.returncode != 0:
        raise RuntimeError(
            f"failed to preprocess {def_path}:\n{result.stderr}"
        )
    return RE_ADJACENT_STRINGS.sub("", result.stdout)
