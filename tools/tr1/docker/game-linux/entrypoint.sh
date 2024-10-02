#!/usr/bin/env python3
from pathlib import Path

from shared.cli.game_docker_entrypoint import run_script

run_script(
    version=1,
    platform="linux",
    compile_args=[],
    release_zip_files=[
        (Path("/app/build/tr1/linux/TR1X"), "TR1X"),
    ],
    compressable_exes=[
        Path("/app/build/tr1/linux/TR1X"),
    ],
)
