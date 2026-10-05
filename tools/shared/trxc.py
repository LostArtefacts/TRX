"""Read the signatures of the C bridge, trxc.*, from the comments above it.

Each function a module registers carries its signature on the first line of
the comment above it, in the LuaCATS type language the annotations use:

    // trxc.camera.get_room(): integer?
    // trxc.assault.start(track?: trx.assault.Track)
    // trxc.ui.measure(text: string): integer, integer

A value the module sets as a field carries its type instead, on the comment
above the code that sets it:

    // trxc.math.DEG_1: integer

Prose about the binding may follow on the next comment lines.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path

PREFIX = "// trxc."

IDENT = r"[A-Za-z_][A-Za-z0-9_]*"
SIGNATURE_RE = re.compile(
    rf"^// trxc\.(?P<path>{IDENT}(?:\.{IDENT})*)"
    r"(?:\((?P<params>.*)\)(?::\s*(?P<returns>.+))?|:\s*(?P<type>.+))$"
)
PARAM_RE = re.compile(rf"^(?P<name>{IDENT}|\.\.\.)(?P<optional>\?)?:\s*(?P<type>.+)$")
TYPE_RE = re.compile(r"^[A-Za-z0-9_.?|\[\]<>(){}:,\" -]+$")

REG_RE = re.compile(r"static const luaL_Reg (\w+)\[\] = \{(.*?)\};", re.S)
REG_ENTRY_RE = re.compile(r'\{\s*"(\w+)",\s*(\w+)\s*\}')
STRUCT_REGISTER_RE = re.compile(r"LUA_Struct_Register\([^;]*?(\w+)\);", re.S)
FUNCTION_RE = re.compile(r"^static int (\w+)\(", re.M)


@dataclass
class Param:
    name: str
    type: str
    optional: bool = False

    def render(self) -> str:
        return f"{self.name}{'?' if self.optional else ''}: {self.type}"


@dataclass
class Signature:
    path: str
    file: Path
    line: int
    params: list[Param] | None = None
    returns: list[str] = field(default_factory=list)
    type: str | None = None

    @property
    def name(self) -> str:
        return self.path.rsplit(".", 1)[-1]

    @property
    def owner(self) -> str:
        return self.path.rsplit(".", 1)[0] if "." in self.path else ""

    def render(self) -> str:
        if self.params is None:
            return self.type or "any"
        params = ", ".join(param.render() for param in self.params)
        returns = f": {', '.join(self.returns)}" if self.returns else ""
        return f"fun({params}){returns}"


@dataclass
class Problem:
    file: Path
    line: int
    message: str

    def __str__(self) -> str:
        return f"{self.file.as_posix()}:{self.line}: {self.message}"


def _split(text: str) -> list[str]:
    # Splits on the commas outside brackets, so `fun(a: integer, b: string)`
    # stays a single type.
    parts, depth, start = [], 0, 0
    for pos, char in enumerate(text):
        if char in "([{<":
            depth += 1
        elif char in ")]}>":
            depth -= 1
        elif char == "," and depth == 0:
            parts.append(text[start:pos].strip())
            start = pos + 1
    parts.append(text[start:].strip())
    return [part for part in parts if part]


def _balanced(text: str) -> bool:
    depth = 0
    for char in text:
        if char in "([{<":
            depth += 1
        elif char in ")]}>":
            depth -= 1
        if depth < 0:
            return False
    return depth == 0


def _check_type(text: str) -> bool:
    return bool(TYPE_RE.match(text)) and _balanced(text)


def parse_line(text: str, file: Path, line: int) -> Signature | Problem:
    match = SIGNATURE_RE.match(text.rstrip())
    if match is None:
        return Problem(file, line, f"not a trxc signature: {text.strip()}")
    sig = Signature(path=match["path"], file=file, line=line)
    if match["type"] is not None:
        if not _check_type(match["type"]):
            return Problem(file, line, f"bad type: {match['type']}")
        sig.type = match["type"].strip()
        return sig
    sig.params = []
    for part in _split(match["params"]):
        param = PARAM_RE.match(part)
        if param is None or not _check_type(param["type"]):
            return Problem(file, line, f"bad parameter: {part}")
        sig.params.append(
            Param(param["name"], param["type"].strip(), bool(param["optional"]))
        )
    for part in _split(match["returns"] or ""):
        if not _check_type(part):
            return Problem(file, line, f"bad return type: {part}")
        sig.returns.append(part)
    return sig


def _comment_above(lines: list[str], index: int) -> list[tuple[int, str]]:
    # Returns the comment lines directly above line `index`, top first.
    found = []
    index -= 1
    while index >= 0 and lines[index].startswith("//"):
        found.append((index, lines[index]))
        index -= 1
    return found[::-1]


def read_file(path: Path) -> tuple[list[Signature], list[Problem]]:
    text = path.read_text(encoding="utf-8")
    lines = text.split("\n")
    signatures: list[Signature] = []
    problems: list[Problem] = []
    by_function: dict[str, list[Signature]] = {}

    for index, line in enumerate(lines):
        if not line.lstrip().startswith(PREFIX):
            continue
        result = parse_line(line.lstrip(), path, index + 1)
        if isinstance(result, Problem):
            problems.append(result)
            continue
        signatures.append(result)

    for index, line in enumerate(lines):
        match = FUNCTION_RE.match(line)
        if match is None:
            continue
        by_function[match[1]] = [
            sig
            for sig in signatures
            if any(sig.line == above + 1 for above, _ in _comment_above(lines, index))
        ]

    methods = set(STRUCT_REGISTER_RE.findall(text))
    for reg in REG_RE.finditer(text):
        if reg[1] in methods:
            continue
        reg_line = text.count("\n", 0, reg.start()) + 1
        for name, function in REG_ENTRY_RE.findall(reg[2]):
            found = by_function.get(function)
            if found is None:
                continue
            if not any(sig.name == name and sig.params is not None for sig in found):
                problems.append(
                    Problem(
                        path,
                        reg_line,
                        f"{function} is registered as `{name}` but has no "
                        f"`// trxc.<module>.{name}(...)` signature above it",
                    )
                )
    return signatures, problems


def read_tree(root: Path) -> tuple[list[Signature], list[Problem]]:
    signatures: list[Signature] = []
    problems: list[Problem] = []
    for path in sorted(root.rglob("*.[ch]"), key=lambda p: p.as_posix()):
        found, issues = read_file(path)
        signatures += found
        problems += issues
    seen: dict[str, Signature] = {}
    for sig in signatures:
        if sig.path in seen and seen[sig.path].render() != sig.render():
            problems.append(
                Problem(
                    sig.file,
                    sig.line,
                    f"trxc.{sig.path} disagrees with {seen[sig.path].file}:"
                    f"{seen[sig.path].line}",
                )
            )
        seen.setdefault(sig.path, sig)
    return list(seen.values()), problems


def to_meta(signatures: list[Signature], generator: str) -> str:
    """Renders the signatures as an EmmyLua/LuaLS definition file."""
    members: dict[str, dict[str, str]] = {"": {}}
    for sig in signatures:
        parts = sig.path.split(".")
        for depth in range(1, len(parts)):
            owner = ".".join(parts[: depth - 1])
            child = ".".join(parts[:depth])
            members.setdefault(owner, {})[parts[depth - 1]] = (
                "trxc." + child
            )
            members.setdefault(child, {})
        members.setdefault(sig.owner, {})[sig.name] = sig.render()

    out = [
        "---@meta",
        f"-- Generated by {generator} from the `// trxc.` signatures in",
        "-- src/trx. Do not edit.",
    ]
    for owner in sorted(members):
        name = "trxc" + (f".{owner}" if owner else "")
        out.append("")
        out.append(f"---@class (exact) {name}")
        for member, rendered in sorted(members[owner].items()):
            out.append(f"---@field {member} {rendered}")
        if not owner:
            out.append("trxc = {}")
    return "\n".join(out) + "\n"
