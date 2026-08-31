#!/usr/bin/env python3

"""The shipped object families and links against the engine that reads them.

Membership and the pairs that join two objects are data now, and a name that
answers to nothing stops the game as it starts. The game is not what a commit
runs, so the same reading happens here: every family, link and object a file
names has to be one the engine holds, no family may be left without members,
and no pair may be stated twice.
"""

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools"))

from shared.cdefs import expand_def_file  # noqa: E402

GAME_DIR = ROOT / "src/trx/game"
SHIP_CFG = ROOT / "data/trx/ship/cfg"
SYMBOL_RE = re.compile(r"SYMBOL\((\w+)\)")
COMMENT_RE = re.compile(r"^\s*//.*$", re.MULTILINE)
TRAILING_COMMA_RE = re.compile(r",(\s*[\]}])")


# Read the shipped files here rather than through the JSON5 parser the tools
# use, because the suite runs on a bare python. The files hold nothing but
# comments, strings and trailing commas.
def load_json5(path: Path):
    text = COMMENT_RE.sub("", path.read_text(encoding="utf-8"))
    return json.loads(TRAILING_COMMA_RE.sub(r"\1", text))
LINK_NAME_RE = re.compile(r'X_OBJECT_LINK\(\w+,\s*"(\w+)"\)')


def keys(def_path: Path, prefix: str) -> set[str]:
    text = expand_def_file(def_path, {"X_CATALOG_ID(x)": "SYMBOL(x)"})
    return {
        m.group(1)[len(prefix) :].lower()
        for m in SYMBOL_RE.finditer(text)
        if m.group(1).startswith(prefix)
    }


def check_families(objects: set[str], families: set[str]) -> list[str]:
    path = SHIP_CFG / "object_families.json5"
    stated = load_json5(path)
    failures = []
    for family, members in stated.items():
        if family not in families:
            failures.append(f"there is no family called '{family}'")
            continue
        if not members:
            failures.append(f"'{family}' holds nothing")
        for key in members:
            if key not in objects:
                failures.append(f"'{family}' names no object '{key}'")
        if len(set(members)) != len(members):
            failures.append(f"'{family}' names one object twice")
    for family in sorted(families - set(stated)):
        failures.append(f"'{family}' is stated nowhere")
    return failures


def check_links(objects: set[str], links: set[str]) -> list[str]:
    path = SHIP_CFG / "object_links.json5"
    stated = load_json5(path)
    failures = []
    for link, pairs in stated.items():
        if link not in links:
            failures.append(f"there is no link called '{link}'")
            continue
        if not pairs:
            failures.append(f"'{link}' holds nothing")
        seen = set()
        for pair in pairs:
            if len(pair) != 2:
                failures.append(f"'{link}' has a row of {len(pair)} names")
                continue
            for key in pair:
                if key not in objects:
                    failures.append(f"'{link}' names no object '{key}'")
            if tuple(pair) in seen:
                failures.append(f"'{link}' joins {pair[0]} to {pair[1]} twice")
            seen.add(tuple(pair))
    for link in sorted(links - set(stated)):
        failures.append(f"'{link}' is stated nowhere")
    return failures


def main() -> int:
    objects = keys(GAME_DIR / "catalog/objects.def", "O_")
    families = keys(GAME_DIR / "catalog/families.def", "OBJ_FAMILY_")
    links = set(
        LINK_NAME_RE.findall(
            (GAME_DIR / "objects/links.def").read_text(encoding="utf-8")
        )
    )

    failures = check_families(objects, families) + check_links(objects, links)
    for failure in failures:
        print(f"  FAIL  {failure}", file=sys.stderr)
    if failures:
        return 1

    print("  PASS  every family and link names what the engine holds")
    return 0


if __name__ == "__main__":
    sys.exit(main())
