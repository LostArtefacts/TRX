#!/usr/bin/env python3

"""The embedded Lua source list against the tree it is supposed to mirror.

Which array a Lua file lands in - a module the engine requires, or a script it
runs once - is stated in src/meson.build and nowhere else, so nothing stops a
new file in src/lua/api/ from being left out of it.

Left out, the file is still loaded by the surface tests, which read the tree;
it is simply absent from the binary and from the reference. The suite would be
green and the module would not exist.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
MESON = ROOT / "src/meson.build"


def listed(variable: str, prefix: str) -> set[str]:
    body = re.search(rf"^{variable} = files\((.*?)^\)", MESON.read_text(), re.S | re.M)
    assert body is not None, f"{variable} is not a files() list in src/meson.build"
    # A module in a directory of its own is named by its path, so the whole
    # path past the tree it sits in is what identifies it.
    return {
        Path(m).relative_to(prefix).as_posix()
        for m in re.findall(r"'([^']+)'", body.group(1))
    }


def main() -> int:
    failures = []

    for variable, directory, prefix in (
        ("trx_lua_api_sources", ROOT / "src/lua/api", "lua/api"),
        ("trx_lua_script_sources", ROOT / "src/lua/commands", "lua/commands"),
    ):
        on_disk = {
            path.relative_to(directory).as_posix()
            for path in directory.rglob("*.lua")
        }
        in_meson = listed(variable, prefix)

        for name in sorted(on_disk - in_meson):
            failures.append(
                f"{directory.relative_to(ROOT)}/{name} is not in {variable}, "
                f"so the engine would not ship it"
            )
        for name in sorted(in_meson - on_disk):
            failures.append(f"{variable} names {name}, which does not exist")

    for failure in failures:
        print(f"  FAIL  {failure}", file=sys.stderr)
    if failures:
        return 1

    print("  PASS  every Lua source is embedded")
    return 0


if __name__ == "__main__":
    sys.exit(main())
