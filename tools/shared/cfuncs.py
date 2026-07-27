"""Locate top-level function definitions in a clang-formatted C file.

clang-format gives such a definition a signature at column 0 and an opening
brace alone on a line; initialisers keep their brace on the line that opens
them, so nothing else in a .c file has that shape.
"""

from __future__ import annotations

import re
from collections.abc import Iterator

ATTRIBUTE_RE = re.compile(r"^__attribute__\(\(\w+\)\)\s+")
NAME_RE = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)\s*\(")


def _head(lines: list[str], brace_idx: int) -> int | None:
    # Walk back to the start of the paragraph holding the brace, then forward
    # past any comment lines, which leaves the first line of the signature.
    start = brace_idx
    while start > 0 and lines[start - 1].strip():
        start -= 1
    while start < brace_idx and lines[start].lstrip().startswith("//"):
        start += 1
    if start == brace_idx:
        return None
    head = lines[start]
    if not (head[:1].isalpha() or head.startswith("_")):
        return None
    # A definition names its return type first. A head that opens with the
    # name itself is a macro that expands to one, such as M_GF_HANDLER(x).
    match = NAME_RE.search(ATTRIBUTE_RE.sub("", head))
    if match is not None and match.start() == 0:
        return None
    return start


def definitions(text: str) -> Iterator[tuple[int, str]]:
    """Yield (1-based line number, signature line) for each definition."""
    lines = text.split("\n")
    for i, line in enumerate(lines):
        if line != "{":
            continue
        start = _head(lines, i)
        if start is not None:
            yield start + 1, lines[start]


def is_static(signature: str) -> bool:
    return ATTRIBUTE_RE.sub("", signature).startswith("static ")


def name_of(signature: str) -> str | None:
    match = NAME_RE.search(ATTRIBUTE_RE.sub("", signature))
    return match[1] if match else None
