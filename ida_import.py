from pathlib import Path
from typing import Optional

import idc

PROGRESS_FILE = Path(__file__).parent / "docs" / "progress.txt"


def to_int(source: str) -> Optional[int]:
    if not source.replace("-", ""):
        return None
    if source.startswith(("0x", "0X")):
        source = source[2:]
    return int(source, 16)


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

        if not name.startswith("sub_"):
            default_function_name = f"sub_{offset:x}"
            print(f"renaming 0x{offset:08x} to {name}")
            idc.set_name(offset, name)

        if flags == "+":
            idc.set_color(offset, idc.CIC_FUNC, 0xF0FFEE)
        elif flags == "x":
            idc.set_color(offset, idc.CIC_FUNC, 0xD8D8D8)
        else:
            idc.set_color(offset, idc.CIC_FUNC, idc.DEFCOLOR)
