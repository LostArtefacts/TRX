"""Utilities for working with translation files."""

import re
from collections.abc import Callable, Iterable
from copy import deepcopy
from pathlib import Path
from typing import Any

from shared.json_utils import write_json_to_string

_OBJECT_NAMES_PTR_RE = re.compile(r"^/objects/([^/]+)/(names/\d+|name)$")
REVIEW_MARKER = r"\{review}"


def chunks[T](generator: Iterable[T], n: int) -> Iterable[list[T]]:
    """Yield successive chunks from a generator."""
    chunk = []
    for item in generator:
        chunk.append(item)
        if len(chunk) == n:
            yield chunk
            chunk = []
    if chunk:
        yield chunk


def is_translation_file(path: Path) -> bool:
    """Return True if `path` ends with '-<lang>.json5'."""
    return path.suffix == ".json5" and "-" in path.stem


def is_mod_file(path: Path) -> bool:
    """Return True if `path.parent` belongs to an extension/mod rather than the base game."""
    return "-" in path.parent.name


def find_base_file(path: Path) -> Path:
    """
    Given a translation file path ending in '-<lang>.json5', return the corresponding base file path '<prefix>.json5'.
    """
    name = path.stem
    if "-" not in name:
        raise ValueError(f"{path}: not a translation file (no '-' in stem)")
    prefix, _lang = name.split("-", 1)
    return path.parent / f"{prefix}.json5"


def clean(source: str | list[str] | None) -> str | list[str] | None:
    if not source:
        return source
    if isinstance(source, list):
        return [clean(item) for item in source]
    return source.replace(REVIEW_MARKER, "")


def should_skip_object_name(ptr: str, trans_data: Any) -> bool:
    """
    Return True if ptr is a per-index object names pointer that can be skipped
    because trans_data provides a 'name' or any non-empty 'names' list for that
    object.
    """
    m = _OBJECT_NAMES_PTR_RE.match(ptr)
    if not m or not isinstance(trans_data, dict):
        return False
    obj = m.group(1)
    obj_trans = trans_data.get("objects", {}).get(obj)
    if not isinstance(obj_trans, dict):
        return False
    return clean(obj_trans.get("name")) or clean(obj_trans.get("names", []))


def format_strings_file(source: Any) -> str:
    source = deepcopy(source)
    if source.get("game_strings"):
        source["game_strings"] = dict(sorted(source["game_strings"].items()))
    content = write_json_to_string(source)
    content = (
        """{
    // For usage, refer to the documentation here:
    // https://github.com/LostArtefacts/TRX/blob/stable/docs/GAME_STRINGS.md
    """
        + content[1:].strip()
        + "\n"
    )
    content = re.sub(r'"\n(\s*[\]}])', r'",\n\1', content, flags=re.M)
    return content
