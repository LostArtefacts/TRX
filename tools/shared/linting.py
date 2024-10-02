#!/usr/bin/env python3
import json
import re
from collections.abc import Callable, Iterable
from dataclasses import dataclass
from pathlib import Path


@dataclass
class LintContext:
    root_dir: Path
    versioned_files: list[Path]


@dataclass
class LintWarning:
    path: Path
    message: str
    line: int | None = None

    def __str__(self) -> str:
        prefix = str(self.path)
        if self.line is not None:
            prefix += f":{self.line}"
        return f"{prefix}: {self.message}"


def lint_json_validity(
    context: LintContext, path: Path
) -> Iterable[LintWarning]:
    if path.suffix != ".json":
        return
    try:
        json.loads(path.read_text())
    except json.JSONDecodeError as ex:
        yield LintWarning(path, f"malformed JSON: {ex!s}")


def lint_newlines(context: LintContext, path: Path) -> Iterable[LintWarning]:
    text = path.read_text(encoding="utf-8")
    if not text:
        return
    if not text.endswith("\n"):
        yield LintWarning(path, "missing newline character at end of file")
    if text.endswith("\n\n"):
        yield LintWarning(path, "extra newline character at end of file")


def lint_trailing_whitespace(
    context: LintContext, path: Path
) -> Iterable[LintWarning]:
    if path.suffix == ".md":
        return
    for i, line in enumerate(path.open("r"), 1):
        if line.rstrip("\n").endswith(" "):
            yield LintWarning(path, "trailing whitespace", line=i)


def lint_const_primitives(
    context: LintContext, path: Path
) -> Iterable[LintWarning]:
    if path.suffix != ".h":
        return
    for i, line in enumerate(path.open("r"), 1):
        if re.search(r"const (int[a-z0-9_]*|bool)\b\s*[a-z]", line):
            yield LintWarning(path, "useless const", line=i)
        if re.search(r"\*\s*const", line):
            yield LintWarning(path, "useless const", line=i)


def lint_game_strings(
    context: LintContext, paths: list[Path]
) -> Iterable[LintWarning]:
    def_paths = list(context.root_dir.rglob("**/game_string.def"))
    defs = [
        match.group(1)
        for path in def_paths
        for match in re.finditer(
            r"GS_DEFINE\(([A-Z_]+),.*\)", path.read_text()
        )
    ]
    if not defs:
        return

    path_hints = " or ".join(
        str(path.relative_to(context.root_dir)) for path in def_paths
    )

    for path in paths:
        if path.suffix != ".c":
            continue
        for i, line in enumerate(path.open("r"), 1):
            for match in re.finditer(r"GS\(([A-Z_]+)\)", line):
                def_ = match.group(1)
                if def_ in defs:
                    continue

                yield LintWarning(
                    path,
                    f"undefined game string: {def_}. "
                    f"Make sure it's defined in {path_hints}.",
                    i,
                )


ALL_LINTERS: list[Callable[[LintContext, Path], Iterable[LintWarning]]] = [
    lint_json_validity,
    lint_newlines,
    lint_trailing_whitespace,
    lint_const_primitives,
]

ALL_BULK_LINTERS: list[
    Callable[[LintContext, list[Path]], Iterable[LintWarning]]
] = [
    lint_game_strings,
]


def lint_file(context: LintContext, file: Path) -> Iterable[LintWarning]:
    for linter_func in ALL_LINTERS:
        yield from linter_func(context, file)


def lint_bulk_files(
    context: LintContext, files: list[Path]
) -> Iterable[LintWarning]:
    for linter_func in ALL_BULK_LINTERS:
        yield from linter_func(context, files)
