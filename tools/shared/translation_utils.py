#!/usr/bin/env python3
"""Utilities for working with JSON5 translation files."""
import json
import re
from collections.abc import Callable, Iterable
from pathlib import Path
from typing import Any

try:
    import pyjson5
except ImportError:
    pyjson5 = None


_OBJECT_NAMES_PTR_RE = re.compile(r"^/objects/([^/]+)/names/\d+$")
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


def find_base_file(path: Path) -> Path:
    """
    Given a translation file path ending in '-<lang>.json5', return the corresponding base file path '<prefix>.json5'.
    """
    name = path.stem
    if "-" not in name:
        raise ValueError(f"{path}: not a translation file (no '-' in stem)")
    prefix, _lang = name.rsplit("-", 1)
    return path.parent / f"{prefix}.json5"


def load_json5(path: Path):
    """Load and parse a JSON5 file as Python data structures. Requires `pyjson5`."""
    if pyjson5 is None:
        raise RuntimeError("pyjson5 is required to parse JSON5 files")
    return pyjson5.loads(path.read_text(encoding="utf-8"))


def write_json(path: Path, data) -> None:
    """Serialize `data` as pretty-printed JSON to `path`, with UTF-8 encoding."""
    path.write_text(
        json.dumps(data, ensure_ascii=False, indent=4) + "\n", encoding="utf-8"
    )


class JSONPointers:
    """
    Tiny helper around RFC-6901 JSON Pointers.

        jp = JSONPointers(data)
        leaves = list(jp)      # every reachable leaf pointer
        value  = jp["/foo/0"]  # read
        jp["/foo/0"] = "BAR"    # write (in-place)

    Works with any mix of dict / list containers.
    """

    def __init__(self, data: Any) -> None:
        self._root = data
        self._pointers: list[str] | None = None  # lazy-built

    def __iter__(self) -> list[str]:
        """Return all JSON-Pointer strings that lead to a *leaf* value."""
        if self._pointers is None:
            self._pointers = []
            self._walk(self._root, [])
        yield from self._pointers.copy()

    def get(self, ptr: str, default: Any | None = None) -> Any:
        try:
            return self[ptr]
        except (KeyError, IndexError):
            return default

    def __getitem__(self, ptr: str) -> Any:
        """Return the value at *ptr* (raises KeyError / IndexError if missing)."""
        node = self._root
        for token in self._split(ptr):
            node = node[self._index_or_key(node, token)]
        return node

    def __setitem__(self, ptr: str, value: Any) -> None:
        """Replace the value at *ptr* with *value* (in-place), auto-creating
        missing dict or list containers as needed."""
        tokens = self._split(ptr)
        if not tokens:
            raise ValueError(
                "Cannot replace the root object through set_path()"
            )

        node = self._root
        for idx, token in enumerate(tokens[:-1]):
            next_index = tokens[idx + 1].lstrip("-").isdigit()
            key = self._index_or_key(node, token)
            node = self._ensure_container(node, key, next_index)

        last_key = self._index_or_key(node, tokens[-1])
        if isinstance(node, list):
            while last_key >= len(node):
                node.append(None)
        node[last_key] = value
        # invalidate cached pointers: tree shape may have changed
        self._pointers = None

    def _ensure_container(
        self, node: Any, key: str | int, is_index: bool
    ) -> Any:
        """Ensure node[key] exists as dict or list based on is_index, and return it."""
        if isinstance(node, dict):
            if key not in node or not isinstance(node[key], (dict, list)):
                node[key] = [] if is_index else {}
            return node[key]
        if isinstance(node, list):
            while key >= len(node):
                node.append([] if is_index else {})
            if not isinstance(node[key], (dict, list)):
                node[key] = [] if is_index else {}
            return node[key]
        raise TypeError(f"Cannot traverse into non-container at token '{key}'")

    @property
    def data(self) -> Any:
        return self._root

    def _walk(self, node: Any, path: list[str | int]) -> None:
        """DFS that records every *leaf* pointer."""
        if isinstance(node, dict):
            for k, v in node.items():
                self._walk(v, path + [k])
        elif isinstance(node, list):
            for i, v in enumerate(node):
                self._walk(v, path + [i])
        else:
            self._pointers.append(self._encode(path))  # type: ignore[arg-type]

    @staticmethod
    def _split(ptr: str) -> list[str]:
        if ptr == "":
            return []
        if not ptr.startswith("/"):
            raise ValueError(f"Invalid JSON Pointer: {ptr!r}")
        return [
            token.replace("~1", "/").replace("~0", "~")
            for token in ptr.lstrip("/").split("/")
        ]

    @staticmethod
    def _encode(path: list[str | int]) -> str:
        esc = lambda s: str(s).replace("~", "~0").replace("/", "~1")
        return "/" + "/".join(esc(p) for p in path)

    @staticmethod
    def _index_or_key(node: Any, token: str) -> str | int:
        # if we're in a list and token looks like an int → use int index
        if isinstance(node, list) and token.lstrip("-").isdigit():
            return int(token)
        return token


def clean(source: str | None) -> str | None:
    if not source:
        return source
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
    return clean(obj_trans.get("name")) or any(
        clean(name) for name in obj_trans.get("names", [])
    )
