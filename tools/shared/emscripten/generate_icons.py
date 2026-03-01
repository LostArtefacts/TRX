#!/usr/bin/env python3
"""Generate PWA icons from a source PNG.

Usage:
    python generate_icons.py <input.png> <output_dir>

Produces icon-192.png and icon-512.png using high-quality Lanczos
resampling.  Requires Pillow (pip install Pillow).
"""

import sys
from pathlib import Path

from PIL import Image

SIZES = [192, 512]


def main() -> None:
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input.png> <output_dir>")
        sys.exit(1)

    src = Path(sys.argv[1])
    out_dir = Path(sys.argv[2])

    if not src.exists():
        print(f"Error: source image not found: {src}")
        sys.exit(1)

    out_dir.mkdir(parents=True, exist_ok=True)

    with Image.open(src) as img:
        img = img.convert("RGBA")
        for size in SIZES:
            resized = img.resize((size, size), Image.LANCZOS)
            dest = out_dir / f"icon-{size}.png"
            resized.save(dest, "PNG")
            print(f"  {dest} ({size}x{size})")


if __name__ == "__main__":
    main()
