#!/usr/bin/env python3
from pathlib import Path

from shared.docker import BaseGameEntrypoint


class LinuxEntrypoint(BaseGameEntrypoint):
    BUILD_ROOT = Path("/app/build/linux/")
    COMPILE_ARGS = []
    RELEASE_ZIP_SUFFIX = "Linux"
    RELEASE_ZIP_FILES = [
        (BUILD_ROOT / "TR1X", "TR1X"),
    ]

    def post_compile(self) -> None:
        if self.target == "release":
            self.compress_exe(self.BUILD_ROOT / "TR1X")

if __name__ == "__main__":
    LinuxEntrypoint().run()
