#!/usr/bin/env python3
import re
from dataclasses import dataclass
from enum import StrEnum, auto
from pathlib import Path

FUNC_RE = re.compile(
    r"^"
    r"(?P<ret_type>[^()]+?)\s*"
    r"\s*(?P<call_type>.*?\s)\s*(?P<func_name>\w+)"
    r"\s*"
    r"\((?P<args>.+)\)"
    r";?$"
)

FUNC_PTR_RE = re.compile(
    r"^"
    r"(?P<ret_type>.+?)\s*"
    r"\("
    r"\s*\*(?P<call_type>.*?\s)\s*(?P<func_name>\w+)"
    r"(?P<array_def>\[[^\]]*?\]+)?"
    r"\)\s*"
    r"\((?P<args>.+)\)"
    r";?$"
)

VAR_RE = re.compile(
    r"^"
    r"(?P<ret_type>.+?)\s*"
    r"(?P<var_name>\b\w+)\s*"
    r"(?P<array_def>(?:\[[^\]]*?\])*)"
    r"(\s*=\s*(?P<value_def>.*?))?"
    r";?"
    r"(\s*\/\/\s*(?P<comment>.*))?"
    r"$"
)


class SymbolStatus(StrEnum):
    DECOMPILED = auto()
    KNOWN = auto()
    TODO = auto()
    UNUSED = auto()


class ProgressFileSection(StrEnum):
    TYPES = "types"
    FUNCTIONS = "functions"
    VARIABLES = "variables"


@dataclass
class Symbol:
    offset: int
    signature: str
    size: int | None = None
    flags: str = ""

    @property
    def offset_str(self) -> str:
        return f"0x{self.offset:08X}"

    @property
    def is_decompiled(self) -> bool:
        return "+" in self.flags

    @property
    def is_called(self) -> bool:
        return "*" in self.flags

    @property
    def is_unused(self) -> bool:
        return "x" in self.flags

    @property
    def is_known(self) -> bool:
        return not re.search(r"(\s|^)sub_", self.signature)

    @property
    def status(self) -> SymbolStatus:
        if self.is_decompiled:
            return SymbolStatus.DECOMPILED
        elif self.is_unused:
            return SymbolStatus.UNUSED
        elif self.is_known:
            return SymbolStatus.KNOWN
        return SymbolStatus.TODO


@dataclass
class ProgressFile:
    types: list[str]
    functions: list[Symbol]
    variables: list[Symbol]


def to_int(source: str) -> int | None:
    source = source.strip()
    if source.startswith("/*"):
        source = source[2:]
    if source.endswith("*/"):
        source = source[:-2]
    source = source.strip()
    if not source.replace("-", ""):
        return None
    if source.startswith(("0x", "0X")):
        source = source[2:]
    return int(source, 16)


def parse_progress_file(path: Path) -> ProgressFile:
    result = ProgressFile(types=[], functions=[], variables=[])

    type_contents = ""

    section: ProgressFileSection | None = None
    for line in path.read_text(encoding="utf-8").splitlines():
        if match := re.match("^# ([A-Z]+)$", line.strip()):
            section_name = match.group(1).lower()
            if section_name in list(ProgressFileSection):
                section = ProgressFileSection(section_name)

        if line.strip().startswith("#"):
            continue

        if section == ProgressFileSection.TYPES:
            type_contents += line + "\n"

        line = line.strip()
        if not line:
            continue

        if section == ProgressFileSection.FUNCTIONS:
            offset, size, flags, signature = re.split(r"\s+", line, maxsplit=3)
            result.functions.append(
                Symbol(
                    signature=signature,
                    offset=to_int(offset),
                    size=to_int(size),
                    flags=flags,
                )
            )

        if section == ProgressFileSection.VARIABLES:
            offset, flags, signature = re.split(r"\s+", line, maxsplit=2)
            result.variables.append(
                Symbol(
                    signature=signature,
                    offset=to_int(offset),
                    flags=flags,
                )
            )

    result.types = [
        definition
        for definition in re.split(r"\n\n(?!\s)", type_contents, flags=re.M)
        if definition.strip()
    ]
    return result
