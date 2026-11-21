"""Find the forward declarations of static functions in a clang-formatted C file.

A forward declaration such as `static void M_Foo(ITEM *item);` is needed only
where the file names the function before its definition: in a function that
calls it, as in mutual recursion, or in a table of handlers. A declaration with
no use between it and the definition does no work, and the function can simply
be defined in its place.

Any mention of the name between the two counts as a use, so a call, a pointer
in an initialiser and a macro body all keep the declaration. A use that the
file does not spell out, such as a name a macro builds by token pasting, is
not seen; a `lint: keep forward declaration` comment on the declaration or on
the line above it keeps the declaration all the same.
"""

from __future__ import annotations

import re
from collections.abc import Iterator
from dataclasses import dataclass

from .cfuncs import definitions

MARKER = "lint: keep forward declaration"

# A declaration names no storage and ends in a parameter list. A pointer to a
# function is storage, and writes its name in parentheses.
FORWARD_RE = re.compile(r"^static\b(?![^(]*\(\s*\*)[^=]*\([^;]*\)\s*;\s*$", re.S)
CALL_RE = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)\s*\(")
WORD_RE = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")
MACRO_RE = re.compile(r"^#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)\(", re.M)
HIDDEN_RE = re.compile(
    r"//[^\n]*|/\*.*?\*/|\"(?:\\.|[^\"\\\n])*\"|'(?:\\.|[^'\\\n])*'", re.S
)


@dataclass
class Forward:
    line: int
    name: str


def code_only(text: str) -> str:
    """Return the text with comments and literals blanked and lines kept."""
    return HIDDEN_RE.sub(lambda match: re.sub(r"[^\n]", " ", match[0]), text)


def function_macros(text: str) -> set[str]:
    """Return the names of the function-like macros that the file defines."""
    return set(MACRO_RE.findall(text))


def function_name(signature: str, macros: set[str]) -> str | None:
    """Return the name that a declaration or a definition gives its function.

    A macro from `macros` that expands to the signature, such as
    `M_DECLARE_HANDLER_FUNC(M_Foo)`, takes the name as its first argument.
    """
    for match in CALL_RE.finditer(signature):
        name = match[1]
        if name == "__attribute__":
            continue
        if name not in macros:
            return name
        inner = WORD_RE.match(signature, match.end())
        return inner[0] if inner is not None else None
    return None


def forwards(code: str) -> Iterator[tuple[int, int, str]]:
    """Yield (first index, last index, text) of each forward declaration."""
    lines = code.split("\n")
    depth = 0
    continued = False
    idx = 0
    while idx < len(lines):
        raw = lines[idx]
        if depth == 0 and not continued and raw.startswith("static "):
            end = idx
            while end < len(lines) - 1 and not lines[end].rstrip().endswith(
                (";", "{", "}")
            ):
                end += 1
            body = "\n".join(lines[idx : end + 1])
            if "{" not in body and FORWARD_RE.match(body):
                yield idx, end, body
                idx = end + 1
                continue
        continued = raw.endswith("\\")
        depth += raw.count("{") - raw.count("}")
        idx += 1


def _defined(text: str, lines: list[str], known: set[str]) -> dict[str, int]:
    # Maps each function name to the index of its first definition.
    result: dict[str, int] = {}
    for line_num, _ in definitions(text):
        end = line_num - 1
        while end < len(lines) and lines[end] != "{":
            end += 1
        name = function_name(" ".join(lines[line_num - 1 : end]), known)
        if name is not None:
            result.setdefault(name, line_num - 1)
    return result


def unneeded(text: str) -> Iterator[Forward]:
    """Yield each forward declaration with no use before the definition."""
    code = code_only(text)
    lines = code.split("\n")
    raw_lines = text.split("\n")
    known = function_macros(code)
    defined = _defined(text, lines, known)
    for first, last, body in forwards(code):
        name = function_name(body, known)
        if name is None or defined.get(name, -1) <= last:
            continue
        if any(MARKER in line for line in raw_lines[max(first - 1, 0) : last + 1]):
            continue
        if name in WORD_RE.findall("\n".join(lines[last + 1 : defined[name]])):
            continue
        yield Forward(first + 1, name)
