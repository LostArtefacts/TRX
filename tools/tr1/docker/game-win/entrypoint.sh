#!/usr/bin/env python3
from pathlib import Path

from libtrx.cli.game_docker_entrypoint import run_script

run_script(
    ship_dir=Path("/app/data/tr1/ship/"),
    build_root=Path("/app/build/win/"),
    compile_args=[
        "--cross",
        "/app/tools/tr1/docker/game-win/meson_linux_mingw32.txt",
    ],
    release_zip_filename="TR1X-{version}-Windows.zip",
    release_zip_files=[
        (Path("/app/build/win/TR1X.exe"), "TR1X.exe"),
        (
            Path("/app/tools/tr1/config/out/TR1X_ConfigTool.exe"),
            "TR1X_ConfigTool.exe",
        ),
    ],
    compressable_exes=[
        Path("/app/build/win/TR1X.exe"),
    ],
)
