import ctypes
from pathlib import Path
from typing import Any, Dict, List, Optional

import ida_typeinf
import idaapi
import idc

REPO_DIR = Path(__file__).parent.parent
DOCS_DIR = REPO_DIR / "docs"
TYPES_FILE = DOCS_DIR / "ida_types.h"
VARIABLES_FILE = DOCS_DIR / "ida_variables.txt"
PROGRESS_FILE = DOCS_DIR / "progress.txt"


def to_int(source: str) -> Optional[int]:
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


def import_types() -> None:
    print(f"Importing type information from {TYPES_FILE}:")
    error_count = idaapi.idc_parse_types(str(TYPES_FILE), idc.PT_FILE)
    print(f"    done ({error_count} errors)")


def import_variables() -> None:
    print(f"Importing variables information from {VARIABLES_FILE}:")
    with VARIABLES_FILE.open("r") as handle:
        for line in handle:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            offset, decl = line.split(maxsplit=1)
            offset = to_int(offset)
            if True:
                print(f"    renaming 0x{offset:08x} to {decl}")

                til = idaapi.get_idati()
                ti = idaapi.tinfo_t()

                name = idaapi.parse_decl(ti, til, decl, idaapi.PT_VAR)
                if name.startswith("_"):
                    name = name[1:]
                if not name.startswith('dword_'):
                    idaapi.set_name(offset, name)
                idaapi.apply_tinfo(offset, ti, 0)
    print("    done")


def import_functions() -> None:
    print(f"Importing types information from {PROGRESS_FILE}:")
    with PROGRESS_FILE.open("r", encoding="utf-8") as handle:
        for line in handle:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            name, offset, size, flags = line.split(maxsplit=3)
            offset = to_int(offset)
            size = to_int(size)

            # ignore inline functions
            if offset is None:
                continue

            old_name = idc.get_name(offset)
            if old_name != name and not name.startswith("sub_"):
                print(f"    renaming {old_name} to {name}")
                idc.set_name(offset, name)

            if flags == "+":
                idc.set_color(offset, idc.CIC_FUNC, 0xF0FFEE)
            elif flags == "x":
                idc.set_color(offset, idc.CIC_FUNC, 0xD8D8D8)
            else:
                idc.set_color(offset, idc.CIC_FUNC, idc.DEFCOLOR)
    print("    done")


if __name__ == "__main__":
    import_types()
    import_variables()
    import_functions()
