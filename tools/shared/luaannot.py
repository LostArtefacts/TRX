"""Read the trx.* API surface from the ---@ annotations in src/lua/trx.

A module built with helpers.lua declares what each member is and does in
annotations next to its code. This parses the subset of the EmmyLua annotation
format those modules use, and returns the surface the docs tool renders and the
strict-mode signatures are generated from.
"""

from __future__ import annotations

import re
import textwrap
from dataclasses import dataclass, field
from pathlib import Path

PRIMITIVES = {
    "any",
    "boolean",
    "function",
    "integer",
    "nil",
    "number",
    "string",
    "table",
}

CATEGORIES = (
    "modules",
    "namespaces",
    "containers",
    "functions",
    "types",
    "enums",
    "constants",
    "properties",
    "numbers",
    "units",
)


class AnnotationError(Exception):
    pass


@dataclass
class Block:
    """A run of --- lines and the statement it sits on, if any."""

    path: Path
    line: int
    text: list[str] = field(default_factory=list)
    tags: list[tuple[str, str]] = field(default_factory=list)
    statement: str | None = None
    statement_line: int = 0

    def tag(self, name: str) -> str | None:
        for key, value in self.tags:
            if key == name:
                return value
        return None

    def all(self, name: str) -> list[str]:
        return [value for key, value in self.tags if key == name]

    def where(self) -> str:
        return f"{self.path}:{self.line}"


# Types


def split_top(text: str, sep: str) -> list[str]:
    """Split `text` on `sep` where no bracket is open."""
    parts, depth, start = [], 0, 0
    for i, ch in enumerate(text):
        if ch in "(<[{":
            depth += 1
        elif ch in ")>]}":
            depth -= 1
        elif ch == sep and depth == 0:
            parts.append(text[start:i].strip())
            start = i + 1
    parts.append(text[start:].strip())
    return parts


def read_type(text: str) -> tuple[str, str]:
    """Split a tag's rest into the type expression and what follows it."""
    depth = 0
    for i, ch in enumerate(text):
        if ch in "(<[{":
            depth += 1
        elif ch in ")>]}":
            depth -= 1
        elif ch == " " and depth == 0:
            # A union may be written with spaces around its bars.
            rest = text[i:].lstrip()
            before = text[:i].rstrip()
            if rest.startswith("|") or before.endswith("|"):
                continue
            # A function type's return follows a colon.
            if before.endswith(":") or rest.startswith(":"):
                continue
            return text[:i].strip(), text[i:].strip()
    return text.strip(), ""


def api_name(name: str) -> str:
    if name in PRIMITIVES:
        return name
    if not name.startswith("trx."):
        raise AnnotationError(f"type '{name}' is neither a primitive nor trx.*")
    return name[len("trx.") :]


def parse_type(text: str, shapes: dict[str, list[dict]] | None = None) -> dict:
    """An annotation type as a dump spec: type, list, nullable, params."""
    text = text.strip()
    spec: dict = {}
    if text.endswith("?"):
        spec["nullable"] = True
        text = text[:-1].strip()
    if text.startswith("(") and text.endswith(")"):
        inner = parse_type(text[1:-1], shapes)
        return {**inner, **spec}
    if text.endswith("[]"):
        inner = parse_type(text[:-2], shapes)
        inner["list"] = True
        return {**inner, **spec}
    union = split_top(text, "|")
    if len(union) > 1:
        parts = [part for part in union if part.strip() != "nil"]
        if len(parts) < len(union):
            spec["nullable"] = True
        if len(parts) == 1:
            return {**parse_type(parts[0], shapes), **spec}
        spec["type"] = [parse_type(part, shapes)["type"] for part in parts]
        return spec
    if text.startswith("fun("):
        close = matching(text, 3)
        params = []
        for part in split_top(text[4:close], ","):
            if not part:
                continue
            name, _, kind = part.partition(":")
            name = name.strip()
            param = {"name": name.rstrip("?"), **parse_type(kind or "any", shapes)}
            if name.endswith("?") or param.get("nullable"):
                param["optional"] = True
            params.append(param)
        spec["type"] = "function"
        spec["params"] = params
        return spec
    if text.startswith("table<"):
        spec["type"] = "table"
        return spec
    name = api_name(text)
    if shapes is not None and name in shapes:
        spec["type"] = "table"
        spec["fields"] = shapes[name]
        return spec
    spec["type"] = name
    return spec


def matching(text: str, open_at: int) -> int:
    depth = 0
    for i in range(open_at, len(text)):
        if text[i] == "(":
            depth += 1
        elif text[i] == ")":
            depth -= 1
            if depth == 0:
                return i
    raise AnnotationError(f"unbalanced parentheses in '{text}'")


# Lua string expressions, as an enum's documentation table writes them.

STRING_START = re.compile(r"""\s*(?:"|'|\[=*\[)""")


def read_string(text: str, pos: int) -> tuple[str, int]:
    """Read one Lua string literal at `pos`; return its value and end."""
    while text[pos].isspace():
        pos += 1
    quote = text[pos]
    if quote in "\"'":
        out, i = [], pos + 1
        escapes = {"n": "\n", "t": "\t", "\\": "\\", '"': '"', "'": "'"}
        while text[i] != quote:
            if text[i] == "\\":
                out.append(escapes.get(text[i + 1], text[i + 1]))
                i += 2
            else:
                out.append(text[i])
                i += 1
        return "".join(out), i + 1
    level = re.match(r"\[(=*)\[", text[pos:])
    if level is None:
        raise AnnotationError(f"expected a string at '{text[pos:pos + 20]}'")
    close = "]" + level.group(1) + "]"
    start = pos + len(level.group(0))
    end = text.index(close, start)
    body = text[start:end]
    if body.startswith("\n"):
        body = body[1:]
    return body, end + len(close)


def read_enum_table(text: str) -> dict[str, str]:
    """The constants of an enum table literal, each written as
    `NAME = h.IntegerConstant,` and described by the `---` lines above it."""
    out: dict[str, str] = {}
    described: list[str] = []
    for line in text[text.index("{") + 1 :].splitlines():
        line = line.strip()
        if line.startswith("---"):
            described.append(line[3:].strip())
        elif (m := re.match(r"([A-Za-z_][A-Za-z0-9_]*)\s*=", line)) is not None:
            out[m.group(1)] = " ".join(part for part in described if part)
            described = []
    return out


# Blocks


DOC_LINE = re.compile(r"^\s*---(?!-+\s*$)(.*)$")
TAG = re.compile(r"^@([A-Za-z_.]+)\s*(.*)$")


def blocks_of(path: Path, source: str) -> list[Block]:
    lines = source.splitlines()
    blocks: list[Block] = []
    i = 0
    while i < len(lines):
        m = DOC_LINE.match(lines[i])
        if m is None:
            i += 1
            continue
        block = Block(path=path, line=i + 1)
        # A tag's description continues on the lines below it that are
        # indented by two spaces or more; a bare --- between two of them
        # breaks a paragraph. A fenced example is never a continuation.
        continues = False
        fenced = False
        blank = False
        while i < len(lines):
            m = DOC_LINE.match(lines[i])
            if m is None:
                break
            body = m.group(1)
            tag = TAG.match(body)
            if FENCE.match(body):
                fenced = not fenced
            if tag is not None and not fenced:
                block.tags.append((tag.group(1), tag.group(2).strip()))
                continues, blank = True, False
            elif continues and not fenced and body.startswith("  ") and body.strip():
                name, value = block.tags[-1]
                joint = "\n\n" if blank else "\n"
                block.tags[-1] = (name, value + joint + body.strip())
                blank = False
            elif continues and not fenced and not body.strip():
                if blank:
                    block.text.append("")
                blank = True
            else:
                if blank:
                    block.text.append("")
                continues, blank = False, False
                block.text.append(body)
            i += 1
        if blank:
            block.text.append("")
        if i < len(lines) and lines[i].strip() and not lines[i].lstrip().startswith("--"):
            # The statement runs to the end of its table literal, if it opens
            # one, so a documentation table is read whole.
            start = i
            statement = [lines[i]]
            depth = lines[i].count("{") - lines[i].count("}")
            while depth > 0 and i + 1 < len(lines):
                i += 1
                statement.append(lines[i])
                depth += lines[i].count("{") - lines[i].count("}")
            block.statement = "\n".join(statement)
            block.statement_line = start + 1
        blocks.append(block)
    return blocks


FENCE = re.compile(r"^\s*```")


def prose(block: Block) -> tuple[str | None, list[str]]:
    """The description and the fenced examples of a block."""
    text: list[str] = []
    examples: list[str] = []
    code: list[str] | None = None
    for line in block.text:
        if FENCE.match(line):
            if code is None:
                code = []
            else:
                examples.append(textwrap.dedent("\n".join(code)))
                code = None
            continue
        if code is not None:
            code.append(line)
        else:
            text.append(line)
    if code is not None:
        raise AnnotationError(f"{block.where()}: unclosed code fence")
    description = textwrap.dedent("\n".join(text)).strip("\n")
    description = re.sub(r"\n{3,}", "\n\n", description)
    return (description or None), examples


def split_desc(rest: str) -> tuple[str, str]:
    kind, desc = read_type(rest)
    return kind, desc.lstrip("#").strip()


# The parser


MODULE = re.compile(r'^local\s+(\w+)\s*=\s*h\.module\("([\w.]+)"\)')
HANDLE = re.compile(r'^local\s+(\w+)\s*=\s*h\.handle\("([\w.]+)"')
CLASS = re.compile(r'^local\s+(\w+)\s*=\s*h\.class\("([\w.]+)"')
DOC_TABLE = re.compile(r"^local\s+(\w+)\s*=\s*\{")
FUNCTION = re.compile(r"^function\s+([\w.]+)([.:])(\w+)\s*\(")
ASSIGN = re.compile(r"^([\w.]+)\.(\w+)\s*=\s*(.*)$", re.DOTALL)
CONTAINER_CALL = re.compile(r'^h\.container\("([\w.]+)"')
HELPER_ARG = re.compile(r'^h\.(\w+)\("([\w.]+)"')
# `local W = trx.ui.widgets`: a file adding to a module or namespace that
# another file declares.
BINDING = re.compile(r"^local\s+(\w+)\s*=\s*trx\.([\w.]+)\s*$", re.MULTILINE)


def table_keys(text: str, start: int) -> list[str]:
    """The keys written at the top level of the table that opens at `start`."""
    keys, depth, i = [], 0, start
    key = re.compile(r"\s*([A-Za-z_]\w*)\s*=(?!=)")
    while i < len(text):
        ch = text[i]
        if ch in "\"'":
            _, i = read_string(text, i)
            continue
        if ch == "{":
            depth += 1
            if depth == 1:
                m = key.match(text, i + 1)
                if m:
                    keys.append(m.group(1))
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return keys
        elif ch == "," and depth == 1:
            m = key.match(text, i + 1)
            if m:
                keys.append(m.group(1))
        elif text.startswith("--", i):
            i = text.find("\n", i)
            if i < 0:
                break
        i += 1
    return keys


def spec_table(statement: str, name: str | None = None) -> list[str]:
    """The keys of a helper call's spec table, or of the table one of its keys
    holds."""
    start = statement.find("{")
    if start < 0:
        return []
    if name is None:
        return table_keys(statement, start)
    m = re.search(rf"\b{name}\s*=\s*{{", statement[start:])
    if m is None:
        return []
    return table_keys(statement, start + m.end() - 1)


@dataclass
class Ref:
    """A default that names a constant, such as an enum value, rather than
    spelling out a literal."""

    path: str


def literal(text: str):
    """A Lua literal as written after ---@trx.default."""
    text = text.strip()
    if text in ("true", "false"):
        return text == "true"
    if text == "nil":
        return None
    if re.fullmatch(r"-?\d+", text):
        return int(text)
    if re.fullmatch(r"-?\d*\.\d+", text):
        return float(text)
    if text[:1] in "\"'":
        value, _ = read_string(text, 0)
        return value
    if re.fullmatch(r"[\w.]+", text):
        return Ref(api_name(text))
    raise AnnotationError(f"cannot read the default '{text}'")


class Parser:
    def __init__(self) -> None:
        self.surface: dict[str, list[dict]] = {key: [] for key in CATEGORIES}
        self.shapes: dict[str, list[dict]] = {}
        # The local variables a file binds to a module, a namespace or a type.
        self.vars: dict[str, tuple[str, str]] = {}
        self.types: dict[str, dict] = {}
        self.owners: set[str] = set()
        # Owners a file reached without declaring them, checked once every
        # file has been read.
        self.reached: list[tuple[Block, str]] = []

    def fail(self, block: Block, message: str) -> None:
        raise AnnotationError(f"{block.where()}: {message}")

    # What a block says

    def field_spec(self, rest: str) -> dict:
        name, _, rest = rest.partition(" ")
        kind, desc = split_desc(rest)
        spec = {"name": name.rstrip("?"), **parse_type(kind, self.shapes)}
        if name.endswith("?"):
            spec["optional"] = True
        if desc:
            spec["description"] = desc
        return spec

    @staticmethod
    def readonly_of(block: Block) -> set[str]:
        names: set[str] = set()
        for rest in block.all("trx.readonly"):
            names.update(n for n in re.split(r"[\s,]+", rest) if n)
        return names

    def fields_of(self, block: Block) -> list[dict]:
        fields = [self.field_spec(rest) for rest in block.all("field")]
        known = {f["name"] for f in fields}
        unknown = self.readonly_of(block) - known
        if unknown:
            self.fail(block, f"read-only names no field: {', '.join(sorted(unknown))}")
        self.apply_defaults(block, fields)
        for rest in block.all("trx.deprecated"):
            name, _, note = rest.partition(" ")
            self.named(block, fields, name)["deprecated"] = note.strip() or True
        return fields

    def named(self, block: Block, specs: list[dict], name: str) -> dict:
        for spec in specs:
            if spec["name"] == name:
                return spec
        self.fail(block, f"no parameter or field named '{name}'")
        return {}

    def apply_defaults(self, block: Block, specs: list[dict]) -> None:
        for rest in block.all("trx.default"):
            name, _, value = rest.partition(" ")
            self.named(block, specs, name)["default"] = literal(value)

    def params_of(self, block: Block) -> list[dict]:
        params = []
        for rest in block.all("param"):
            name, _, rest = rest.partition(" ")
            kind, desc = split_desc(rest)
            spec = {"name": name.rstrip("?"), **parse_type(kind, self.shapes)}
            if name.endswith("?") or spec.get("nullable"):
                spec["optional"] = True
            if desc:
                spec["description"] = desc
            params.append(spec)
        self.apply_defaults(block, params)
        for rest in block.all("trx.arg"):
            path, _, desc = rest.partition(" ")
            outer, _, inner = path.partition(".")
            callback = self.named(block, params, outer)
            arg = self.named(block, callback.get("params", []), inner)
            arg["description"] = desc.strip()
        return params

    def returns_of(self, block: Block) -> dict | list | None:
        returns = []
        for rest in block.all("return"):
            kind, desc = split_desc(rest)
            spec = parse_type(kind, self.shapes)
            if desc:
                spec["description"] = desc
            returns.append(spec)
        if not returns:
            return None
        return returns[0] if len(returns) == 1 else returns

    def described(self, block: Block, entry: dict) -> dict:
        description, examples = prose(block)
        if description:
            entry["description"] = description
        if examples:
            entry["examples"] = examples
        if block.tag("deprecated") is not None:
            entry["deprecated"] = block.tag("deprecated") or True
        return entry

    def callable_entry(self, block: Block, entry: dict, params=None) -> dict:
        self.described(block, entry)
        params = self.params_of(block) if params is None else params
        if params:
            entry["params"] = params
        returns = self.returns_of(block)
        if returns is not None:
            entry["returns"] = returns
        return entry

    # Where a name lands

    def resolve(self, block: Block, expr: str) -> tuple[str, str]:
        """What a dotted expression such as M.log names: ("owner", path) for
        a module or a namespace, ("type", path) for a type."""
        first, _, rest = expr.partition(".")
        if first not in self.vars:
            self.fail(block, f"'{first}' is no module, namespace or type")
        kind, path = self.vars[first]
        if rest:
            if kind != "owner":
                self.fail(block, f"'{expr}' reaches inside a type")
            path = f"{path}.{rest}"
        if kind == "owner":
            self.reached.append((block, path))
        return kind, path

    # The file

    def first_pass(self, blocks: list[Block]) -> None:
        # Shapes and records are written anywhere in a file and named from
        # anywhere, so they are read before the members that use them.
        parents: dict[str, str] = {}
        for block in blocks:
            if block.statement is not None:
                continue
            declared = block.tag("class")
            if declared is None or declared.partition(":")[0].split()[-1] == "trx":
                continue
            if block.tag("trx.record") is not None:
                continue
            name, parent = self.class_parent(block)
            self.shapes[name] = self.fields_of(block)
            if parent is not None:
                parents[name] = api_name(parent)
        for name in parents:
            self.shapes[name] = self.inherited(name, parents)

    def inherited(self, name: str, parents: dict[str, str]) -> list[dict]:
        own = self.shapes[name]
        parent = parents.get(name)
        if parent not in self.shapes:
            return own
        names = {f["name"] for f in own}
        base = self.inherited(parent, parents)
        return [f for f in base if f["name"] not in names] + own

    def parse(self, path: Path) -> None:
        source = path.read_text(encoding="utf-8")
        for var, owner in BINDING.findall(source):
            self.vars[var] = ("owner", owner)
        blocks = blocks_of(path, source)
        self.first_pass(blocks)
        for block in blocks:
            self.read_block(block)

    def finish(self) -> None:
        for block, path in self.reached:
            if path not in self.owners:
                self.fail(block, f"'trx.{path}' is no declared module or namespace")

    def read_block(self, block: Block) -> None:
        statement = block.statement or ""
        if block.tag("alias") is not None:
            self.read_alias(block)
            return
        declared = block.tag("class")
        if declared is not None and declared.split()[-1] == "trx":
            return
        if declared is not None and block.statement is None:
            if block.tag("trx.record") is not None:
                self.read_record(block)
            return
        if (m := MODULE.match(statement)) is not None:
            self.read_module(block, m.group(1), m.group(2))
        elif (m := HANDLE.match(statement)) is not None:
            self.read_type(block, m.group(1), m.group(2), handle=True)
        elif (m := CLASS.match(statement)) is not None:
            self.read_type(block, m.group(1), m.group(2), handle=False)
        elif block.tag("enum") is not None and DOC_TABLE.match(statement):
            self.read_enum(block)
        elif (m := FUNCTION.match(statement)) is not None:
            self.read_function(block, m)
        elif (m := CONTAINER_CALL.match(statement)) is not None:
            self.read_container(block, m.group(1))
        elif (m := ASSIGN.match(statement)) is not None:
            self.read_assign(block, m)
        else:
            self.fail(block, "annotations attached to nothing the parser knows")

    def class_parent(self, block: Block) -> tuple[str, str | None]:
        """The class a block declares, and the parent it extends: the first
        one that is not a table<...>, which only says the class is indexed."""
        declared = block.tag("class") or ""
        declared = re.sub(r"^\(\w+\)\s*", "", declared)
        name, _, parents = declared.partition(":")
        for parent in split_top(parents, ","):
            if parent and not parent.startswith("table<"):
                return api_name(name.strip()), parent
        return api_name(name.strip()), None

    def read_alias(self, block: Block) -> None:
        name, _, base = block.tag("alias").partition(" ")
        path = api_name(name)
        description, examples = prose(block)
        if block.tag("trx.bulk") is not None:
            entry: dict = {"path": path, "bulk": True, "values": []}
            source = block.tag("trx.catalog")
            if source is not None:
                def_name, _, prefix = source.partition(" ")
                entry["catalog"] = (def_name, prefix.strip())
            if description:
                entry["description"] = description
            if examples:
                entry["examples"] = examples
            self.surface["enums"].append(entry)
            return
        entry = {"path": path, "description": description}
        unit = block.tag("trx.unit")
        if unit is not None:
            entry["type"] = base.strip()
            spellings = [s.strip() for s in unit.split(",") if s.strip()]
            if spellings:
                entry["spellings"] = spellings
            self.surface["units"].append(entry)
            return
        start = block.tag("trx.base")
        if start is not None:
            entry["base"] = int(start)
        self.surface["numbers"].append(entry)

    def properties_of(self, block: Block, owner: str) -> None:
        readonly = self.readonly_of(block)
        for spec in self.fields_of(block):
            spec["writable"] = spec["name"] not in readonly
            prop = {"path": f"{owner}.{spec.pop('name')}"}
            if spec.pop("optional", False):
                spec["nullable"] = True
            prop.update(spec)
            self.surface["properties"].append(prop)

    def read_module(self, block: Block, var: str, name: str) -> None:
        self.vars[var] = ("owner", name)
        self.owners.add(name)
        order = block.tag("trx.module")
        if order is None:
            self.fail(block, "a module needs ---@trx.module <order> [title]")
        number, _, title = order.partition(" ")
        entry: dict = {"name": name, "order": int(number)}
        if title.strip():
            entry["title"] = title.strip()
        description, _ = prose(block)
        if description:
            entry["description"] = description
        _, parent = self.class_parent(block)
        if parent is not None:
            entry["instance_type"] = api_name(parent)
        self.surface["modules"].append(entry)
        self.properties_of(block, name)

    def read_type(self, block: Block, var: str, path: str, handle: bool) -> None:
        name, parent = self.class_parent(block)
        if name != path:
            self.fail(block, f"'{path}' needs ---@class trx.{path}")
        description, _ = prose(block)
        fields = self.fields_of(block)
        readonly = self.readonly_of(block)
        for spec in fields:
            spec.pop("nullable", None)
            spec["writable"] = spec["name"] not in readonly
        computed = set(spec_table(block.statement or "", "extensions")) if handle else set()
        extensions = [
            {k: v for k, v in f.items() if k in ("name", "type", "description")}
            for f in fields
            if f["name"] in computed
        ]
        fields = [f for f in fields if f["name"] not in computed]
        operators = []
        for rest in block.all("trx.operator"):
            op, _, desc = rest.partition(" ")
            operators.append({"name": op, "description": desc.strip()})
        entry: dict = {
            "path": path,
            "description": description,
            "handle": handle,
            "fields": sorted(fields, key=lambda f: f["name"]),
            "methods": [],
            "operators": sorted(operators, key=lambda o: o["name"]),
            "extensions": sorted(extensions, key=lambda f: f["name"]),
        }
        if parent is not None:
            entry["extends"] = api_name(parent)
        self.vars[var] = ("type", path)
        self.types[path] = entry
        self.surface["types"].append(entry)

    def read_record(self, block: Block) -> None:
        name, _ = self.class_parent(block)
        description, examples = prose(block)
        entry: dict = {
            "path": name,
            "description": description,
            "handle": False,
            "record": True,
            "fields": sorted(self.fields_of(block), key=lambda f: f["name"]),
            "methods": [],
            "operators": [],
            "extensions": [],
        }
        for spec in entry["fields"]:
            spec.pop("nullable", None)
        if examples:
            entry["examples"] = examples
        self.surface["types"].append(entry)

    def read_enum(self, block: Block) -> None:
        name = block.tag("enum")
        docs = read_enum_table(block.statement or "")
        if block.tag("trx.bulk") is not None:
            # Named one by one so an editor completes them, and described as a
            # whole in the reference.
            entry: dict = {
                "path": api_name(name.split()[-1]),
                "bulk": True,
                "values": [],
                "names": sorted(docs),
                "count": len(docs),
            }
            self.described(block, entry)
            self.surface["enums"].append(entry)
            return
        entry = {
            "path": api_name(name.split()[-1]),
            "bulk": False,
            "values": [{"name": k, "description": v} for k, v in docs.items()],
        }
        self.described(block, entry)
        self.surface["enums"].append(entry)

    def read_function(self, block: Block, m: re.Match) -> None:
        owner, sep, name = m.group(1), m.group(2), m.group(3)
        kind, path = self.resolve(block, owner)
        if kind == "type" and sep == ":":
            method = self.callable_entry(block, {"name": name})
            entry = self.types[path]
            entry["methods"].append(method)
            entry["methods"].sort(key=lambda x: x["name"])
            return
        if kind == "owner" and sep == ".":
            entry = self.callable_entry(block, {"path": f"{path}.{name}"})
            self.surface["functions"].append(entry)
            return
        self.fail(block, f"'{owner}{sep}{name}' is neither a function nor a method")

    def read_container(self, block: Block, path: str, value: str = "") -> None:
        declared = block.tag("type")
        m = re.match(r"table<(.+)>$", declared or "")
        if m is None:
            self.fail(block, "a collection is typed ---@type table<key, value>")
        key, value_type = split_top(m.group(1), ",")
        module, _, member = path.partition(".")
        entry: dict = {
            "module": module,
            "key": parse_type(key, self.shapes),
            "value": parse_type(value_type, self.shapes),
        }
        key_description = block.tag("trx.key")
        if key_description:
            entry["key"]["description"] = key_description
        if member:
            entry["member"] = member
        entry["countable"] = "count" in spec_table(block.statement or "")
        self.described(block, entry)
        self.surface["containers"].append(entry)

    def read_assign(self, block: Block, m: re.Match) -> None:
        kind, owner = self.resolve(block, m.group(1))
        name, value = m.group(2), m.group(3).lstrip()
        path = f"{owner}.{name}"
        if kind != "owner":
            self.fail(block, f"assignment to the type {owner}")
        helper = HELPER_ARG.match(value)
        if helper is not None and helper.group(2) != path:
            self.fail(block, f"{path} is declared as '{helper.group(2)}'")
        used = helper.group(1) if helper is not None else None
        if used == "namespace":
            self.read_namespace(block, m.group(1) + "." + name, path, value)
        elif used == "const":
            self.read_const(block, path)
        elif used == "container":
            self.read_container(block, path)
        elif used in ("enum", "catalog"):
            return
        else:
            self.read_function_value(block, path)

    def read_namespace(self, block: Block, expr: str, path: str, value: str) -> None:
        self.owners.add(path)
        declared, _ = self.class_parent(block)
        if declared != path:
            self.fail(block, f"{path} needs ---@class trx.{path}")
        entry: dict = {
            "path": path,
            "callable": "," in value.split(")")[0],
        }
        if block.tag("trx.implicit") is not None:
            entry["implicit"] = True
        if entry["callable"]:
            self.callable_entry(block, entry)
        else:
            self.described(block, entry)
        self.surface["namespaces"].append(entry)
        self.properties_of(block, path)

    def read_const(self, block: Block, path: str) -> None:
        entry: dict = {"path": path}
        declared = block.tag("type")
        if declared is not None:
            entry["type"] = parse_type(declared)["type"]
        sample = block.tag("trx.sample")
        value = block.tag("trx.value")
        if sample is not None:
            entry["value"] = literal(sample)
            entry["sample"] = True
        elif value is not None:
            entry["value"] = literal(value)
        else:
            self.fail(block, f"{path} needs ---@trx.value or ---@trx.sample")
        self.described(block, entry)
        self.surface["constants"].append(entry)

    def read_function_value(self, block: Block, path: str) -> None:
        declared = block.tag("type")
        if declared is None:
            self.fail(block, f"{path} needs ---@type")
        spec = parse_type(declared, self.shapes)
        if spec.get("type") != "function":
            self.fail(block, f"cannot tell what {path} is from its ---@type")
        params = self.params_of(block) or spec.get("params")
        entry = self.callable_entry(block, {"path": path}, params)
        self.surface["functions"].append(entry)


def parse_files(paths: list[Path]) -> dict[str, list[dict]]:
    parser = Parser()
    for path in paths:
        parser.parse(path)
    parser.finish()
    return parser.surface


def annotated_modules(root: Path) -> list[Path]:
    """The files under `root` that annotate the API: every one built with
    helpers.lua, apart from the engine's own modules under internal/."""
    # Sorted as text, so ui.lua comes before ui/screens.lua, as the engine
    # loads them.
    return sorted(
        (
            path
            for path in root.rglob("*.lua")
            if 'require("trx.internal.helpers")' in path.read_text(encoding="utf-8")
            and "internal" not in path.relative_to(root).parts
        ),
        key=lambda path: path.as_posix(),
    )


def catalog_names(surface: dict[str, list[dict]], root: Path) -> None:
    """Name the identities of each catalog, from the .def file the engine
    builds it from. Reading one runs the C preprocessor, which the docs tool
    has and a build that only embeds the modules may not."""
    from shared.cdefs import expand_def_file

    for entry in surface["enums"]:
        source = entry.pop("catalog", None)
        if source is None:
            continue
        def_name, prefix = source
        text = expand_def_file(
            root / "src/trx/game/catalog" / def_name,
            {"X_CATALOG_ID(x)": "CATALOG_ID x"},
        )
        names = {
            name[len(prefix):] if name.startswith(prefix) else name
            for name in re.findall(r"CATALOG_ID (\w+)", text)
        }
        entry["names"] = sorted(names)
        entry["count"] = len(names)


def named_defaults(node) -> None:
    """Spell each default that names a constant as the constant it names."""
    if isinstance(node, list):
        for item in node:
            named_defaults(item)
        return
    if not isinstance(node, dict):
        return
    if isinstance(node.get("default"), Ref):
        node["default"] = {"constant": node["default"].path}
    for value in node.values():
        named_defaults(value)


def finish(surface: dict[str, list[dict]]) -> None:
    """Shape the parsed surface for the docs: count each enum, and drop what
    only the strict-mode signatures read."""
    for entry in surface["enums"]:
        if not entry["bulk"]:
            entry["count"] = len(entry["values"])
    for entry in surface["types"]:
        entry.pop("record", None)
    for entry in surface["constants"]:
        entry.pop("sample", None)
    named_defaults(surface)


# Signatures for strict mode

SIGNATURE_KEYS = ("name", "type", "optional", "default", "list", "fields")


@dataclass
class LuaCode:
    code: str


def bare(spec: dict) -> dict:
    out = {key: spec[key] for key in SIGNATURE_KEYS if key in spec}
    if "fields" in out:
        out["fields"] = [bare(f) for f in out["fields"]]
    if isinstance(out.get("default"), Ref):
        # Read once the modules have loaded: the value is C's.
        out["default"] = LuaCode(
            f"function() return trx.{out['default'].path} end"
        )
    return out


def signatures(surface: dict[str, list[dict]]) -> dict:
    """What strict mode checks the annotated members against.

    `types` gives the value a number, a unit or an enum stands for, since a
    script passes the number itself. `members` lists what each module and
    namespace table holds, for the seal to report anything else.
    """
    functions = {
        f["path"]: [bare(p) for p in f.get("params", [])]
        for f in surface["functions"]
    }
    calls = {
        n["path"]: [bare(p) for p in n.get("params", [])]
        for n in surface["namespaces"]
        if n.get("callable")
    }
    methods = {
        f"{t['path']}.{m['name']}": [bare(p) for p in m.get("params", [])]
        for t in surface["types"]
        for m in t["methods"]
    }
    types = {n["path"]: "integer" for n in surface["numbers"]}
    types.update({u["path"]: u["type"] for u in surface["units"]})
    types.update({e["path"]: "integer" for e in surface["enums"]})
    records = {
        t["path"]: [bare(f) for f in t["fields"]]
        for t in surface["types"]
        if t.get("record")
    }
    properties = {}
    for p in surface["properties"]:
        spec = bare({k: v for k, v in p.items() if k != "name"})
        if p.get("nullable"):
            spec["optional"] = True
        properties[p["path"]] = spec
    containers = {}
    for c in surface["containers"]:
        path = c["module"] + (f".{c['member']}" if c.get("member") else "")
        containers[path] = bare(c["key"])
    members: dict[str, list[str]] = {}

    def member(path: str) -> None:
        owner, _, name = path.rpartition(".")
        members.setdefault(owner, []).append(name)

    for category in ("functions", "constants", "enums"):
        for entry in surface[category]:
            member(entry["path"])
    for entry in surface["namespaces"]:
        member(entry["path"])
    for c in surface["containers"]:
        if c.get("member"):
            member(f"{c['module']}.{c['member']}")
    for owner in members:
        members[owner].sort()

    # What the annotations say takes a write, for the seal to hold the
    # runtime to: a property and a field are read-only exactly where the
    # annotations say so.
    writable: dict[str, dict[str, bool]] = {}
    for p in surface["properties"]:
        owner, _, name = p["path"].rpartition(".")
        writable.setdefault(owner, {})[name] = p["writable"]
    for t in surface["types"]:
        if t.get("record"):
            continue
        fields = {}
        for f in t["fields"]:
            fields[f["name"]] = f["writable"]
        writable[t["path"]] = fields
    constants = {
        c["path"]: c["value"] for c in surface["constants"] if not c.get("sample")
    }
    return {
        "functions": functions,
        "calls": calls,
        "methods": methods,
        "types": types,
        "records": records,
        "properties": properties,
        "containers": containers,
        "members": members,
        "writable": writable,
        "constants": constants,
    }


def to_lua(value, indent: str = "") -> str:
    """A Lua literal for plain JSON-like data."""
    inner = indent + "  "
    if isinstance(value, LuaCode):
        return value.code
    if value is None:
        return "nil"
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, (int, float)):
        return repr(value)
    if isinstance(value, str):
        return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'
    if isinstance(value, list):
        if not value:
            return "{}"
        items = [f"{inner}{to_lua(v, inner)}," for v in value]
        return "{\n" + "\n".join(items) + f"\n{indent}}}"
    if isinstance(value, dict):
        if not value:
            return "{}"
        items = [
            f"{inner}[{to_lua(k)}] = {to_lua(v, inner)},"
            for k, v in sorted(value.items())
        ]
        return "{\n" + "\n".join(items) + f"\n{indent}}}"
    raise TypeError(f"cannot write {type(value).__name__} as Lua")


def signatures_module(root: Path) -> str:
    """The source of trx.internal.signatures, for the modules under `root`."""
    surface = parse_files(annotated_modules(root))
    return (
        "-- Generated from the annotations in src/lua/trx. Do not edit.\n"
        f"return {to_lua(signatures(surface))}\n"
    )
