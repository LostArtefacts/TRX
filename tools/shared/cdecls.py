"""Read the top-level declarations of a clang-formatted C file in order.

docs/CODING_GUIDELINES.md gives them one: defines, local module types, static
module variables, static module functions, public functions. It also allows a
deviation the code shape forces, and one such shape is common enough to name:
a table of function pointers, or any other initialiser naming static functions,
has to sit below the functions it names or every one of them needs a forward
declaration first. A declaration reaching back like that is left alone.
"""

from __future__ import annotations

import re
from collections.abc import Iterator
from dataclasses import dataclass

from .cfuncs import definitions, is_static, name_of

RANK = {
    "define": 0,
    "type": 1,
    "static variable": 2,
    "static function": 3,
    "public function": 4,
}

DEFINE_RE = re.compile(r"^#\s*define\s")
TYPE_RE = re.compile(r"^(typedef|struct|enum|union)\b")
STATIC_RE = re.compile(r"^static\b")
VAR_NAME_RE = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)\s*(\[|=|;)")
WORD_RE = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")
# `static bool M_Parse(const char *line);` names no storage. The guidelines
# give a forward declaration as a reason to deviate from the order, so it is
# not read as a declaration at all. A pointer to a function is storage, and
# writes its name in parentheses - `static void (*m_Handlers[N])(...)` - which
# is what tells the two apart.
FORWARD_RE = re.compile(r"^static\b(?![^(]*\(\s*\*)[^=]*\([^;]*\)\s*;\s*$", re.S)


@dataclass
class Declaration:
    line: int
    kind: str
    label: str
    text: str


def _without_literals(line: str) -> str:
    # A brace inside a string, a character or a line comment opens nothing.
    line = re.sub(r"\\.", "", line)
    line = re.sub(r'"[^"]*"', '""', line)
    line = re.sub(r"'[^']*'", "''", line)
    return line.split("//")[0]


def _label(kind: str, line: str) -> str:
    if kind == "define":
        return f"the define of {line.split()[1].split('(')[0]}"
    if kind == "static variable":
        match = VAR_NAME_RE.search(line)
        return match[1] if match else "a static variable"
    return "a type"


def _span(lines: list[str], idx: int) -> tuple[str, int]:
    """The declaration's own text, and the index of the line after it."""
    if DEFINE_RE.match(lines[idx]):
        end = idx
        while end < len(lines) - 1 and lines[end].endswith("\\"):
            end += 1
        return "\n".join(lines[idx : end + 1]), end + 1

    depth, end = 0, idx
    while end < len(lines):
        stripped = _without_literals(lines[end])
        depth += stripped.count("{") - stripped.count("}")
        if depth <= 0 and stripped.rstrip().endswith(";"):
            break
        end += 1
    return "\n".join(lines[idx : end + 1]), end + 1


def declarations(text: str) -> Iterator[Declaration]:
    """Yield each top-level declaration in the order it is written."""
    lines = text.split("\n")
    functions = {
        line_num: ("static function" if is_static(sig) else "public function", sig)
        for line_num, sig in definitions(text)
    }

    depth = 0
    continued = False
    for idx, raw in enumerate(lines):
        line_num = idx + 1
        if depth == 0 and not continued:
            if line_num in functions:
                kind, sig = functions[line_num]
                yield Declaration(line_num, kind, name_of(sig) or kind, sig)
            elif DEFINE_RE.match(raw) or TYPE_RE.match(raw) or STATIC_RE.match(raw):
                kind = (
                    "define"
                    if DEFINE_RE.match(raw)
                    else "type" if TYPE_RE.match(raw) else "static variable"
                )
                body, _ = _span(lines, idx)
                if kind == "static variable" and FORWARD_RE.match(body):
                    continue
                yield Declaration(line_num, kind, _label(kind, raw), body)
        continued = raw.endswith("\\")
        stripped = _without_literals(raw)
        depth += stripped.count("{") - stripped.count("}")


def _is_scoped(decl: Declaration, undefined: set[str]) -> bool:
    # A define the file takes back with #undef reaches only as far as the lines
    # between the two, and what sits there is written expecting it. An X-macro
    # feeding the #include below it is the usual shape. Moving the define out
    # of that stretch changes what those lines mean, so it stays where it is.
    if decl.kind != "define":
        return False
    return decl.text.split()[1].split("(")[0] in undefined


def out_of_order(text: str) -> Iterator[tuple[Declaration, Declaration]]:
    """Yield (declaration, the higher-ranked one above it) for each violation."""
    static_funcs = {
        name_of(sig): line_num
        for line_num, sig in definitions(text)
        if is_static(sig)
    }
    undefined = set(re.findall(r"^#\s*undef\s+([A-Za-z_][A-Za-z0-9_]*)", text, re.M))

    highest = None
    for decl in declarations(text):
        if highest is not None and RANK[decl.kind] < RANK[highest.kind]:
            reached = {
                word
                for word in WORD_RE.findall(decl.text)
                if static_funcs.get(word, decl.line) < decl.line
            }
            if not reached and not _is_scoped(decl, undefined):
                yield decl, highest
            continue
        if highest is None or RANK[decl.kind] > RANK[highest.kind]:
            highest = decl
