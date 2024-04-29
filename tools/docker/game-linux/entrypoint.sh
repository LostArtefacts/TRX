#!/usr/bin/env python3
from pathlib import Path

from libtrx.cli.game_docker_entrypoint import run_script

run_script(
    ship_dir=Path("/app/data/ship/"),
    build_root=Path("/app/build/linux/"),
    compile_args=[],
    release_zip_filename="TR1X-{version}-Linux.zip",
    release_zip_files=[
        (Path("/app/build/linux/TR1X"), "TR1X"),
    ],
    compressable_exes=[
        Path("/app/build/linux/TR1X"),
    ],
)
