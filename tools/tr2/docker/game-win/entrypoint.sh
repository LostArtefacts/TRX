#!/usr/bin/env python3
from pathlib import Path

from shared.cli.game_docker_entrypoint import run_script

run_script(
    version=2,
    platform="win",
    compile_args=[
        "--cross",
        "/app/tools/tr2/docker/game-win/meson_linux_mingw32.txt",
    ],
    release_zip_files=[
        (Path("/app/build/tr2/win/TR2X.exe"), "TR2X.exe"),
        (Path("/app/build/tr2/win/TR2X.dll"), "TR2X.dll"),
        (
            Path("/app/tools/tr2/config/out/TR2X_ConfigTool.exe"),
            "TR2X_ConfigTool.exe",
        ),
    ],
    compressable_exes=[
        Path("/app/build/tr2/win/TR2X.exe"),
        Path("/app/build/tr2/win/TR2X.dll"),
    ],
)
