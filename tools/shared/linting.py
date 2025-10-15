#!/usr/bin/env python3
import json
import re
import sys
from collections import defaultdict
from collections.abc import Callable, Iterable
from dataclasses import dataclass
from pathlib import Path

from shared.paths import PROJECT_PATHS

# enable importing translation_utils from this directory
sys.path.insert(0, str(Path(__file__).resolve().parent))

import jsonschema
from json_utils import JSONPointers, load_json5
from translation_utils import (
    REVIEW_MARKER,
    clean,
    find_base_file,
    is_translation_file,
    should_skip_object_name,
)

SHARED_PROJECT = "libtrx"
CHILD_PROJECTS = ["tr1", "tr2"]
PROJECTS = [SHARED_PROJECT] + CHILD_PROJECTS
RE_GAME_STRING_DEFINE = re.compile(r"GS_DEFINE\(([A-Z0-9_]+),\s*\"(.+)\"\)")
RE_GAME_STRING_DEFINE_VAL = re.compile(
    r'GS_DEFINE\(\s*([A-Z0-9_]+)\s*,\s*"([^"]*)"\)'
)
RE_GAME_STRING_USAGE = re.compile(r"GS(?:_ID|_PTR)?\(([A-Z0-9_]+)\)")
RE_UI_SETTING_USAGE = re.compile(
    r"(?:X_UI_CFG_MANUAL\(\s*|X_UI_CFG[A-Z0-9_]*\([^(),]*,\s*)([A-Z0-9_]+)[,)]",
    flags=re.M | re.DOTALL,
)
RE_ENUM_USAGE = re.compile(r"\b([A-Z0-9_]+)\b")


@dataclass
class LintContext:
    root_dir: Path
    versioned_files: list[Path]


@dataclass
class GameString:
    project: str
    path: Path
    key: str
    text: str


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
        if re.search(r"\*\s*const(?!\s?\*)", line):
            yield LintWarning(path, "useless const", line=i)


def lint_meson_build_sort_order(
    context: LintContext, path: Path
) -> Iterable[LintWarning]:
    if path.name != 'meson.build' or path.parent.name == 'dwarfstack':
        return
    pattern = re.compile(r'\bsources\s*=\s*\[(.*?)\]', re.S)
    match = pattern.search(path.read_text())

    if not match:
        yield LintWarning(path, "unable to find sources array")
    else:
        block = match.group(1)
        lines = [l.strip().strip(',') for l in block.splitlines() if l.strip()]
        if not lines:
            yield LintWarning(path, "unable to parse sources array")
        else:
            clean = [l.strip("'") for l in lines]
            init = [l for l in clean if l == 'init']
            resources = [l for l in clean if l == 'resources']
            middle = [l for l in clean if l not in ('init', 'resources')]
            is_sorted = clean == init + sorted(middle) + resources
            if not is_sorted:
                yield LintWarning(path, "source list is not ordered")


def lint_untranslated_game_strings(
    context: LintContext, path: Path
) -> Iterable[LintWarning]:
    # Warn when translation JSON5 files are missing or have extra keys compared to their base file
    if not is_translation_file(path):
        return
    try:
        base_file = find_base_file(path)
    except ValueError:
        return
    if not base_file.exists():
        return

    try:
        base_data = load_json5(base_file)
    except Exception as exc:
        yield LintWarning(base_file, f"unable to parse base JSON5: {exc}")
        return
    try:
        trans_data = load_json5(path)
    except Exception as exc:
        yield LintWarning(path, f"unable to parse JSON5: {exc}")
        return
    base_ptr = JSONPointers(base_data)
    trans_ptr = JSONPointers(trans_data)
    base_paths = list(base_ptr)

    for ptr in base_paths:
        if should_skip_object_name(ptr, trans_data):
            continue
        # Only warn for missing translations in dialects if they're already present
        if "extends" in trans_data and ptr not in trans_ptr:
            continue
        if not clean(trans_ptr.get(ptr)):
            yield LintWarning(path, f"untranslated key '{ptr}'")
    # Warn about extra translation keys not present in the base file (unused by the game)
    base_set = set(base_paths)
    for ptr in trans_ptr:
        if ptr == "/extends":
            continue
        if should_skip_object_name(ptr, trans_data):
            continue
        if ptr not in base_set:
            yield LintWarning(path, f"extra translation key '{ptr}'")


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


def get_all_game_strings(context: LintContext) -> Iterable[GameString]:
    for project, def_paths in get_project_game_strings_paths(context).items():
        for path in def_paths:
            for match in re.finditer(RE_GAME_STRING_DEFINE, path.read_text()):
                yield GameString(
                    project=project,
                    path=path,
                    key=match.group(1),
                    text=match.group(2),
                )


def lint_duplicate_game_strings(context: LintContext) -> Iterable[LintWarning]:
    project_game_strings = defaultdict(lambda: defaultdict(dict))
    for game_string in get_all_game_strings(context):
        project_game_strings[game_string.project][
            game_string.key
        ] = game_string
    for child_project in CHILD_PROJECTS:
        for child_string in project_game_strings.get(
            child_project, []
        ).values():
            shared_string = project_game_strings.get(SHARED_PROJECT, []).get(
                child_string.key
            )
            if shared_string and child_string.text == shared_string.text:
                yield LintWarning(
                    child_string.path,
                    f"duplicate game string definition: {child_string.key} "
                    f"in {child_project} matches {SHARED_PROJECT}",
                )

            sibling_projects = [
                project
                for project in CHILD_PROJECTS
                if project != child_project
            ]
            sibling_strings = [
                project_game_strings.get(sibling_project, []).get(
                    child_string.key
                )
                for sibling_project in sibling_projects
            ]
            if all(
                sibling_string and sibling_string.text == child_string.text
                for sibling_string in sibling_strings
            ):
                yield LintWarning(
                    sibling_strings[0].path,
                    f"duplicate game string definition: {child_string.key} "
                    f"in {child_project} matches all other child projects "
                    "and should be moved to libtrx",
                )


def get_used_strings(
    path: Path, include_enums: bool = False
) -> Iterable[tuple[int, str]]:
    source = re.sub("//.*", "", path.read_text(), flags=re.M)
    for match in re.finditer(RE_GAME_STRING_USAGE, source):
        yield source.count("\n", 0, match.start()) + 1, match.group(1)
    for match in re.finditer(RE_UI_SETTING_USAGE, source):
        yield source.count("\n", 0, match.start()) + 1, match.group(1)
        yield source.count("\n", 0, match.start()) + 1, match.group(
            1
        ) + "_DESCRIPTION"
    if include_enums and path.suffix == ".def":
        for match in re.finditer(RE_ENUM_USAGE, source):
            yield source.count(
                "\n", 0, match.start()
            ) + 1, "ENUM_" + match.group(1)


def lint_undefined_game_strings(
    context: LintContext, paths: list[Path]
) -> Iterable[LintWarning]:
    project_game_strings_paths = get_project_game_strings_paths(context)
    def_string_map: list[set[str]] = defaultdict(set)
    game_strings = list(get_all_game_strings(context))
    assert game_strings
    for game_string in game_strings:
        def_string_map[game_string.project].add(game_string.key)

    for path in paths:
        if path.suffix not in {".c", ".def"}:
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

        for line_num, def_ in get_used_strings(path):
            # For child project: it needs to be defined in libtrx or the child project
            # For libtrx: it needs to be defined in libtrx…
            if any(
                def_ in def_string_map[project]
                for project in relevant_projects
            ):
                continue

            # …or every child project
            if all(
                def_ in def_string_map[project] for project in CHILD_PROJECTS
            ):
                continue

            yield LintWarning(
                path,
                f"undefined game string: {def_}. "
                f"Make sure it's defined in {path_hint}.",
                line_num,
            )


def lint_unused_game_strings(context: LintContext) -> Iterable[LintWarning]:
    project_paths = get_project_paths(context)
    project_game_strings_paths = get_project_game_strings_paths(context)
    game_strings = list(get_all_game_strings(context))

    used_strings = defaultdict(set)
    for path in context.versioned_files:
        if path.suffix not in {".c", ".def"}:
            continue

        relevant_project = get_relevant_project(context, path)
        for line_num, used_string in get_used_strings(
            path, include_enums=True
        ):
            used_strings[relevant_project].add(used_string)

    for game_string in game_strings:
        relevant_projects = {
            SHARED_PROJECT: PROJECTS,
            **{
                child_project: [child_project, SHARED_PROJECT]
                for child_project in CHILD_PROJECTS
            },
        }[game_string.project]
        used_projects = {
            rel_project
            for rel_project in relevant_projects
            if game_string.key in used_strings[rel_project]
        }
        if len(used_projects) == 0:
            yield LintWarning(
                game_string.path, f"unused game string: {game_string.key}."
            )
        elif (
            game_string.project == SHARED_PROJECT
            and SHARED_PROJECT not in used_projects
            and len(used_projects) == 1
        ):
            yield LintWarning(
                game_string.path,
                f"game string used only in a single child project: {game_string.key} ({used_projects!s}).",
            )


def lint_game_flow_schema(context: LintContext):
    for project, paths in PROJECT_PATHS.items():
        schema = load_json5(paths.docs_dir / "gameflow.schema.json")
        validator_cls = jsonschema.validators.validator_for(schema)
        validator = validator_cls(schema=schema)
        game_flow_paths = paths.shipped_data_dir.rglob("**/gameflow.json5")
        for game_flow_path in game_flow_paths:
            data = load_json5(game_flow_path)
            for error in validator.iter_errors(instance=data):
                yield LintWarning(game_flow_path, error)


ALL_FILE_LINTERS: list[
    Callable[[LintContext, Path], Iterable[LintWarning]]
] = [
    lint_json_validity,
    lint_newlines,
    lint_trailing_whitespace,
    lint_const_primitives,
    lint_untranslated_game_strings,
    lint_meson_build_sort_order,
]

ALL_BULK_LINTERS: list[
    Callable[[LintContext, list[Path]], Iterable[LintWarning]]
] = [
    lint_undefined_game_strings,
]

ALL_REPO_LINTERS: list[
    Callable[[LintContext, list[Path]], Iterable[LintWarning]]
] = [
    lint_duplicate_game_strings,
    lint_unused_game_strings,
    lint_game_flow_schema,
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
