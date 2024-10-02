#!/usr/bin/env python3
from pathlib import Path

from libtrx.cli.game_docker_entrypoint import run_script

run_script(
    ship_dir=Path("/app/data/ship/"),
    build_root=Path("/app/build/win/"),
    compile_args=[
        "--cross",
        "/app/tools/docker/game-win/meson_linux_mingw32.txt",
    ],
    release_zip_filename="TR2X-{version}-Windows.zip",
    release_zip_files=[
        (Path("/app/build/win/TR2X.exe"), "TR2X.exe"),
        (Path("/app/build/win/TR2X.dll"), "TR2X.dll"),
        (
            Path("/app/tools/config/out/TR2X_ConfigTool.exe"),
            "TR2X_ConfigTool.exe",
        ),
    ],
    compressable_exes=[
        Path("/app/build/win/TR2X.exe"),
        Path("/app/build/win/TR2X.dll"),
    ],
)
