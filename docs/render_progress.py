#!/usr/bin/python3
import typing as T
from dataclasses import dataclass
import re
from pathlib import Path

MAX_X = 50
SQUARE_SIZE = 10
SQUARE_MARGIN = 2
DOCS_DIR = Path(__file__).parent
PROGRESS_TXT_FILE = DOCS_DIR / "progress.txt"
PROGRESS_SVG_FILE = DOCS_DIR / "progress.svg"


@dataclass
class Function:
    name: str
    offset: int
    size: int
    flags: str

    @property
    def decompiled(self):
        return "+" in self.flags


def collect_functions() -> T.Iterable[Function]:
    for line in PROGRESS_TXT_FILE.open():
        if line.startswith("#"):
            continue
        func_name, offset, size, flags = re.split(r"\s+", line.strip())
        yield Function(
            name=func_name,
            offset=int(offset, 16),
            size=int(size, 16),
            flags=flags,
        )


def main() -> None:
    functions = list(collect_functions())

    svg_width = (
        min(MAX_X, len(functions)) * (SQUARE_SIZE + SQUARE_MARGIN)
        + SQUARE_MARGIN
    )
    svg_height = ((len(functions) + MAX_X - 1) // MAX_X) * (
        SQUARE_SIZE + SQUARE_MARGIN
    ) + SQUARE_MARGIN

    with PROGRESS_SVG_FILE.open("w") as handle:
        print(
            f'<svg version="1.1" '
            f'width="{svg_width}" '
            f'height="{svg_height}" '
            f'xmlns="http://www.w3.org/2000/svg">',
            file=handle,
        )

        for i, function in enumerate(functions):
            x = (i % MAX_X) * (SQUARE_SIZE + SQUARE_MARGIN)
            y = (i // MAX_X) * (SQUARE_SIZE + SQUARE_MARGIN)
            color = "green" if function.decompiled else "pink"
            print(
                f"<rect "
                f'width="{SQUARE_SIZE}" '
                f'height="{SQUARE_SIZE}" '
                f'x="{x}" '
                f'y="{y}" '
                f'fill="{color}"/>',
                file=handle,
            )

        print("</svg>", file=handle)


if __name__ == "__main__":
    main()
