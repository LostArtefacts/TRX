#!/usr/bin/env python3
from pathlib import Path

from shared.docker import BaseGameEntrypoint


class WindowsEntrypoint(BaseGameEntrypoint):
    BUILD_ROOT = Path("/app/build/win/")
    COMPILE_ARGS = [
        "--cross",
        "/app/tools/docker/game-win/meson_linux_mingw32.txt",
    ]
    RELEASE_ZIP_SUFFIX = "Windows"
    RELEASE_ZIP_FILES = [
        (BUILD_ROOT / "TR1X.exe", "TR1X.exe"),
        (Path("/app/tools/config/out/TR1X_ConfigTool.exe"), "TR1X_ConfigTool.exe"),
    ]

    def post_compile(self) -> None:
        if self.target == "release":
            for path in self.BUILD_ROOT.glob("*.exe"):
                self.compress_exe(path)


if __name__ == "__main__":
    WindowsEntrypoint().run()
