#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
from copy import deepcopy
from dataclasses import dataclass
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parent
if str(REPO_ROOT / "tools") not in sys.path:
    sys.path.insert(0, str(REPO_ROOT / "tools"))

from shared.json_utils import load_json5, write_json_to_string  # noqa: E402

SETTING_PREFIX = "SETTING_"
SETTING_SUFFIX_DESCRIPTION = "_DESCRIPTION"
SETTINGS_KEY = "settings"


@dataclass(frozen=True)
class OptionMapping:
    option_name: str
    setting_key: str


def _split_top_level_args(args_text: str) -> list[str]:
    args: list[str] = []
    depth = 0
    cur: list[str] = []
    for ch in args_text:
        if ch == "," and depth == 0:
            args.append("".join(cur).strip())
            cur = []
            continue
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
        cur.append(ch)
    if cur:
        args.append("".join(cur).strip())
    return args


def _load_config_aliases() -> dict[str, str]:
    aliases: dict[str, str] = {}
    map_path = REPO_ROOT / "src/trx/config/map.def"
    if not map_path.exists():
        return aliases

    ex_re = re.compile(
        r'^X_CFG_[A-Z0-9_]+_EX\(\s*"([^"]+)"\s*,\s*([a-z0-9_.]+)\s*,'
    )
    for line in map_path.read_text(encoding="utf-8").splitlines():
        match = ex_re.match(line.strip())
        if not match:
            continue
        canonical_name = match.group(1)
        target_name = match.group(2)
        aliases[target_name] = canonical_name
    return aliases


def _sort_mapping_rec(value: Any) -> Any:
    if isinstance(value, dict):
        if set(value.keys()).issubset({"title", "description"}):
            ordered: dict[str, Any] = {}
            if "title" in value:
                ordered["title"] = _sort_mapping_rec(value["title"])
            if "description" in value:
                ordered["description"] = _sort_mapping_rec(value["description"])
            for key in sorted(value.keys()):
                if key not in ordered:
                    ordered[key] = _sort_mapping_rec(value[key])
            return ordered
        return {key: _sort_mapping_rec(value[key]) for key in sorted(value.keys())}
    if isinstance(value, list):
        return [_sort_mapping_rec(item) for item in value]
    return value


def _format_settings_file(data: dict[str, Any]) -> str:
    source = deepcopy(data)
    if isinstance(source.get("game_strings"), dict):
        source["game_strings"] = _sort_mapping_rec(source["game_strings"])
    if isinstance(source.get("settings"), dict):
        source["settings"] = _sort_mapping_rec(source["settings"])

    content = write_json_to_string(source)
    content = (
        "{\n"
        "    // For usage, refer to the documentation here:\n"
        "    // https://lostartefacts.dev/trx/docs/stable/game_strings\n"
        + content[1:].strip()
        + "\n"
    )
    content = re.sub(r'"\n(\s*[\]}])', r'",\n\1', content, flags=re.M)
    return content


def migrate_def_files(dry_run: bool) -> tuple[list[OptionMapping], list[Path]]:
    mappings: list[OptionMapping] = []
    changed: list[Path] = []
    aliases = _load_config_aliases()
    def_paths = sorted((REPO_ROOT / "src/trx/game/ui/dialogs/setting_tabs").glob("*.def"))

    call_re = re.compile(r"^(?P<indent>\s*)X_UI_CFG_[A-Z0-9_]+\((?P<body>.*)\)(?P<tail>\s*)$")

    for path in def_paths:
        src = path.read_text(encoding="utf-8")
        lines = src.splitlines(keepends=True)
        out_lines: list[str] = []
        file_changed = False
        transformed_rows: list[dict[str, Any] | None] = []

        for line in lines:
            stripped_newline = line[:-1] if line.endswith("\n") else line
            match = call_re.match(stripped_newline)
            if not match:
                transformed_rows.append(None)
                continue

            args = _split_top_level_args(match.group("body"))
            if len(args) < 2:
                transformed_rows.append(None)
                continue

            option_name = args[0]
            canonical_option_name = aliases.get(option_name, option_name)
            maybe_setting = args[1]
            if not maybe_setting.startswith(SETTING_PREFIX):
                transformed_rows.append(None)
                continue

            mappings.append(
                OptionMapping(
                    option_name=canonical_option_name, setting_key=maybe_setting
                )
            )
            transformed_rows.append(
                {
                    "indent": match.group("indent"),
                    "macro": stripped_newline.split("(", 1)[0],
                    "option_name": option_name,
                    "suffix_args": args[2:],
                    "tail": match.group("tail"),
                    "had_newline": line.endswith("\n"),
                    "orig": line,
                }
            )

        max_prefix_len = 0
        for row in transformed_rows:
            if row is None or not row["suffix_args"]:
                continue
            max_prefix_len = max(
                max_prefix_len, len(f"{row['macro']}({row['option_name']},")
            )

        for idx, row in enumerate(transformed_rows):
            if row is None:
                out_lines.append(lines[idx])
                continue
            option_name = row["option_name"]
            suffix_args = row["suffix_args"]
            if suffix_args:
                prefix = f"{row['macro']}({option_name},"
                padding_len = max(1, max_prefix_len - len(prefix) + 1)
                padding = " " * padding_len
                new_args = f"{option_name},{padding}{', '.join(suffix_args)}"
            else:
                new_args = option_name

            new_line = f"{row['indent']}{row['macro']}({new_args}){row['tail']}"
            if row["had_newline"]:
                new_line += "\n"
            out_lines.append(new_line)
            if new_line != row["orig"]:
                file_changed = True

        if file_changed:
            changed.append(path)
            if not dry_run:
                path.write_text("".join(out_lines), encoding="utf-8")

    return mappings, changed


def _ensure_settings_node(root: dict[str, Any]) -> dict[str, Any]:
    settings_node = root.get(SETTINGS_KEY)
    if settings_node is None:
        settings_node = {}
        root[SETTINGS_KEY] = settings_node
    if not isinstance(settings_node, dict):
        raise RuntimeError("settings exists but is not an object")
    return settings_node


def migrate_json5_files(mappings: list[OptionMapping], dry_run: bool) -> list[Path]:
    changed: list[Path] = []
    mapping_by_setting: dict[str, list[str]] = {}
    for mapping in mappings:
        mapping_by_setting.setdefault(mapping.setting_key, []).append(
            mapping.option_name
        )

    for path in sorted((REPO_ROOT / "data").rglob("*strings*.json5")):
        data = load_json5(path)
        if not isinstance(data, dict):
            continue
        game_strings = data.get("game_strings")
        if not isinstance(game_strings, dict):
            continue

        touched = False
        settings_node = _ensure_settings_node(data)

        for setting_key, option_names in mapping_by_setting.items():
            title_val = game_strings.pop(setting_key, None)
            desc_val = game_strings.pop(
                setting_key + SETTING_SUFFIX_DESCRIPTION, None
            )
            if title_val is None and desc_val is None:
                continue

            for option_name in option_names:
                opt_node = settings_node.get(option_name)
                if opt_node is None:
                    opt_node = {}
                    settings_node[option_name] = opt_node
                if not isinstance(opt_node, dict):
                    raise RuntimeError(
                        f"settings.{option_name} exists but is not an object in {path}"
                    )

                if title_val is not None:
                    opt_node["title"] = title_val
                if desc_val is not None:
                    opt_node["description"] = desc_val
            touched = True

        if touched:
            changed.append(path)
            if not dry_run:
                path.write_text(
                    _format_settings_file(data), encoding="utf-8"
                )

    return changed


def migrate_entries_def(mappings: list[OptionMapping], dry_run: bool) -> bool:
    by_setting: dict[str, list[str]] = {}
    for mapping in mappings:
        by_setting.setdefault(mapping.setting_key, []).append(mapping.option_name)

    entries_path = REPO_ROOT / "src/trx/game/game_strings/entries.def"
    src = entries_path.read_text(encoding="utf-8")
    lines = src.splitlines()
    out_lines: list[str] = []
    changed = False

    title_re = re.compile(
        r'^(?P<indent>\s*)GS_DEFINE\((?P<key>SETTING_[A-Z0-9_]+),\s*(?P<value>"(?:\\.|[^"\\])*")\)\s*$'
    )
    desc_re = re.compile(
        r'^(?P<indent>\s*)GS_DEFINE\((?P<key>SETTING_[A-Z0-9_]+)_DESCRIPTION,\s*(?P<value>"(?:\\.|[^"\\])*")\)\s*[;]?\s*$'
    )

    for line in lines:
        title_match = title_re.match(line)
        if title_match:
            options = by_setting.get(title_match.group("key"), [])
            if options:
                for option_name in options:
                    out_lines.append(
                        f'{title_match.group("indent")}GS_DEFINE(settings/{option_name}/title, {title_match.group("value")})'
                    )
                changed = True
                continue

        desc_match = desc_re.match(line)
        if desc_match:
            options = by_setting.get(desc_match.group("key"), [])
            if options:
                for option_name in options:
                    out_lines.append(
                        f'{desc_match.group("indent")}GS_DEFINE(settings/{option_name}/description, {desc_match.group("value")})'
                    )
                changed = True
                continue

        out_lines.append(line)

    if changed and not dry_run:
        entries_path.write_text("\n".join(out_lines) + "\n", encoding="utf-8")

    return changed


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Migrate settings labels/descriptions from SETTING_* keys into game_strings.settings and remove SETTING args from X_UI_CFG macros."
    )
    parser.add_argument("--dry-run", action="store_true", help="Do not write files.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    mappings, def_changed = migrate_def_files(args.dry_run)
    if not mappings:
        print("No X_UI_CFG mappings found that still use SETTING_* labels.")
        return

    entries_changed = migrate_entries_def(mappings, args.dry_run)
    json_changed = migrate_json5_files(mappings, args.dry_run)

    print(f"Mappings processed: {len(mappings)}")
    print(f".def files changed: {len(def_changed)}")
    print(f"entries.def changed: {1 if entries_changed else 0}")
    print(f".json5 files changed: {len(json_changed)}")


if __name__ == "__main__":
    main()
