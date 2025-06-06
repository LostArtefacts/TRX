#!/usr/bin/env python3
import json
import re
from collections import defaultdict
from collections.abc import Callable, Iterable
from dataclasses import dataclass
from pathlib import Path

PROJECTS = ["tr1", "tr2", "libtrx"]
RE_GAME_STRING_DEFINE = re.compile(r"GS_DEFINE\(([A-Z0-9_]+),.*\)")
RE_GAME_STRING_USAGE = re.compile(r"GS(?:_ID)?\(([A-Z0-9_]+)\)")


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


def get_relevant_project(context: LintContext, path: Path) -> str:
    for project, project_path in get_project_paths(context).items():
        if path.absolute().is_relative_to(project_path.absolute()):
            break
    else:
        raise RuntimeError(f"{path}: Unable to get project path")
    return project


def get_project_paths(context: LintContext) -> dict[str, Path]:
    return {
        project: context.root_dir / "src" / project for project in PROJECTS
    }


def get_project_game_strings_paths(
    context: LintContext,
) -> dict[str, list[Path]]:
    return {
        project: list(project_path.rglob("**/game_string.def"))
        for project, project_path in get_project_paths(context).items()
    }


def get_project_game_string_maps(context: LintContext) -> dict[str, set[str]]:
    return {
        project: [
            match.group(1)
            for path in def_paths
            for match in re.finditer(RE_GAME_STRING_DEFINE, path.read_text())
        ]
        for project, def_paths in get_project_game_strings_paths(
            context
        ).items()
    }


def lint_undefined_game_strings(
    context: LintContext, paths: list[Path]
) -> Iterable[LintWarning]:
    project_paths = get_project_paths(context)
    project_game_strings_paths = get_project_game_strings_paths(context)
    def_string_map = get_project_game_string_maps(context)
    if not def_string_map:
        yield LintWarning("Unable to list game string definitions")
        return

    for path in paths:
        if path.suffix != ".c":
            continue

        relevant_projects = [get_relevant_project(context, path), "libtrx"]
        relevant_paths = sum(
            [
                project_game_strings_paths[relevant_project]
                for relevant_project in sorted(relevant_projects)
            ],
            [],
        )
        path_hint = " or ".join(
            str(relevant_path.relative_to(context.root_dir))
            for relevant_path in relevant_paths
        )

        for i, line in enumerate(path.open("r"), 1):
            for match in re.finditer(RE_GAME_STRING_USAGE, line):
                def_ = match.group(1)
                if any(
                    def_ in def_string_map[project]
                    for project in relevant_projects
                ):
                    continue

                yield LintWarning(
                    path,
                    f"undefined game string: {def_}. "
                    f"Make sure it's defined in {path_hint}.",
                    i,
                )


def lint_unused_game_strings(context: LintContext) -> Iterable[LintWarning]:
    project_paths = get_project_paths(context)
    project_game_strings_paths = get_project_game_strings_paths(context)
    project_game_strings_maps = get_project_game_string_maps(context)
    if not project_game_strings_maps:
        yield LintWarning("Unable to list game string definitions")
        return

    used_strings = defaultdict(set)
    for path in context.versioned_files:
        if path.suffix not in {".c", ".def"}:
            continue

        relevant_project = get_relevant_project(context, path)
        for i, line in enumerate(path.open("r"), 1):
            for match in re.finditer(RE_GAME_STRING_USAGE, line):
                used_strings[relevant_project].add(match.group(1))

    for project, defs in project_game_strings_maps.items():
        relevant_projects = {
            "libtrx": PROJECTS,
            "tr1": ["tr1", "libtrx"],
            "tr2": ["tr2", "libtrx"],
        }[project]
        for def_ in defs:
            used_projects = {rel_project
                for rel_project in relevant_projects
                if def_ in used_strings[rel_project]
            }
            if len(used_projects) == 0:
                yield LintWarning(
                    project_paths[project], f"unused game string: {def_}."
                )
            elif project == 'libtrx' and 'libtrx' not in used_projects and len(used_projects) == 1:
                yield LintWarning(
                    project_paths[project], f"game string used only in a single child project: {def_} ({used_projects!s})."
                )


ALL_FILE_LINTERS: list[
    Callable[[LintContext, Path], Iterable[LintWarning]]
] = [
    lint_json_validity,
    lint_newlines,
    lint_trailing_whitespace,
    lint_const_primitives,
]

ALL_BULK_LINTERS: list[
    Callable[[LintContext, list[Path]], Iterable[LintWarning]]
] = [
    lint_undefined_game_strings,
]

ALL_REPO_LINTERS: list[
    Callable[[LintContext, list[Path]], Iterable[LintWarning]]
] = [
    lint_unused_game_strings,
]


def lint_file(context: LintContext, file: Path) -> Iterable[LintWarning]:
    for linter_func in ALL_FILE_LINTERS:
        yield from linter_func(context, file)


def lint_bulk_files(
    context: LintContext, files: list[Path]
) -> Iterable[LintWarning]:
    for linter_func in ALL_BULK_LINTERS:
        yield from linter_func(context, files)


def lint_repo(context: LintContext) -> Iterable[LintWarning]:
    for linter_func in ALL_REPO_LINTERS:
        yield from linter_func(context)
