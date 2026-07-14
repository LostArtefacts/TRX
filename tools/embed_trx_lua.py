#!/usr/bin/env python3

"""Embed the engine's Lua into C, as the two arrays declared in
trx/game/lua/embedded_scripts.h. Which array a file lands in is decided by the
meson source list it is named in, not by anything the loader has to work out.
"""

from __future__ import annotations

import argparse
from pathlib import Path

Script = tuple[str, str, bytes]


def _c_ident_from_path(path: Path, taken: set[str]) -> str:
    raw = path.as_posix()
    out: list[str] = []
    for ch in raw:
        if ch.isalnum():
            out.append(ch)
        else:
            out.append("_")
    # The path is now part of the name, and every character that is not
    # alphanumeric folds to an underscore, so a/b.lua and a-b.lua arrive here as
    # the same identifier.
    base = "m_TrxLua_" + "".join(out)
    ident = base
    suffix = 2
    while ident in taken:
        ident = f"{base}_{suffix}"
        suffix += 1
    taken.add(ident)
    return ident


def _bytes_to_c_array(data: bytes) -> str:
    if not data:
        # An empty initialiser is not C. The loader reads the size, which is zero.
        return "    0"
    per_line = 16
    chunks: list[str] = []
    for i in range(0, len(data), per_line):
        chunk = ", ".join(f"0x{b:02x}" for b in data[i : i + per_line])
        chunks.append("    " + chunk)
    return ",\n".join(chunks)


def _collect(paths: list[Path], root: Path, taken: set[str]) -> list[Script]:
    # A module's path is what its name is derived from, so it is kept bare:
    # items.lua, the module trx.items. A script's keeps its directory.
    scripts: list[Script] = []
    for path in paths:
        rel = path.resolve().relative_to(root.resolve())
        scripts.append(
            (rel.as_posix(), _c_ident_from_path(rel, taken), path.read_bytes())
        )
    return scripts


def _emit_array(name: str, scripts: list[Script]) -> list[str]:
    lines = [f"const LUA_EMBEDDED_SCRIPT {name}[] = {{"]
    for path, c_ident, data in scripts:
        lines.append(
            f'    {{ .path = "{path}", .data = {c_ident}, .size = {len(data)} }},'
        )
    lines.append("    { .path = nullptr, .data = nullptr, .size = 0 },")
    lines.append("};")
    lines.append("")
    return lines


def main() -> int:
    parser = argparse.ArgumentParser(description="Embed TRX Lua scripts into C")
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--module-root", required=True, type=Path)
    parser.add_argument("--modules", nargs="*", default=[], type=Path)
    parser.add_argument("--script-root", required=True, type=Path)
    parser.add_argument("--scripts", nargs="*", default=[], type=Path)
    args = parser.parse_args()

    taken: set[str] = set()
    modules = _collect(args.modules, args.module_root, taken)
    scripts = _collect(args.scripts, args.script_root, taken)

    lines: list[str] = []
    lines.append("// Auto-generated file; do not edit.")
    lines.append("#include <trx/game/lua/embedded_scripts.h>")
    lines.append("")
    for _, c_ident, data in modules + scripts:
        lines.append(f"static const uint8_t {c_ident}[] = {{")
        lines.append(_bytes_to_c_array(data) + ",")
        lines.append("};")
        lines.append("")

    lines += _emit_array("g_LUA_EmbeddedModules", modules)
    lines += _emit_array("g_LUA_EmbeddedRuntimeScripts", scripts)

    args.output.write_text("\n".join(lines), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
