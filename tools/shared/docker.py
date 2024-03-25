import argparse
import os
from pathlib import Path
from subprocess import check_call, check_output, run

from shared.versioning import generate_version
from shared.packaging import create_zip

SHIP_DIR = Path("/app/data/ship")


class BaseGameEntrypoint:
    BUILD_ROOT: Path = ...
    COMPILE_ARGS: list[str] = ...
    STRIP_TOOL = "strip"
    UPX_TOOL = "upx"
    RELEASE_ZIP_SUFFIX: str = ...
    RELEASE_ZIP_FILES: list[tuple[Path, str]] = ...

    def __init__(self) -> None:
        self.target = os.environ.get("TARGET", "debug")

    def run(self) -> None:
        args = self.parse_args()
        args.func(args)

    def parse_args(self) -> argparse.Namespace:
        parser = argparse.ArgumentParser(description="Docker entrypoint")
        subparsers = parser.add_subparsers(dest="action", help="Subcommands")

        compile_parser = subparsers.add_parser(
            "compile", help="Compile action"
        )
        compile_parser.set_defaults(func=self.compile)

        package_parser = subparsers.add_parser(
            "package", help="Package action"
        )
        package_parser.add_argument("-o", "--output", type=Path)
        package_parser.set_defaults(func=self.package)

        args = parser.parse_args()
        if not hasattr(args, "func"):
            args.action = "compile"
            args.func = self.compile
        return args

    def compile(self, args: argparse.Namespace) -> None:
        pkg_config_path = os.environ["PKG_CONFIG_PATH"]

        if not Path("/app/build/linux/build.jinja").exists():
            check_call(
                [
                    "meson",
                    "--buildtype",
                    self.target,
                    *self.COMPILE_ARGS,
                    self.BUILD_ROOT,
                    "--pkg-config-path",
                    pkg_config_path,
                ]
            )

        check_call(["meson", "compile"], cwd=self.BUILD_ROOT)

        self.post_compile()

    def post_compile(self) -> None:
        pass

    def compress_exe(self, path: Path) -> None:
        if run([self.UPX_TOOL, "-t", str(path)]).returncode != 0:
            check_call([self.STRIP_TOOL, str(path)])
            check_call([self.UPX_TOOL, str(path)])

    def package(self, args: argparse.Namespace) -> None:
        if args.output:
            zip_path = args.output
        else:
            version = generate_version()
            zip_path = Path(f"{version}-{self.RELEASE_ZIP_SUFFIX}.zip")
        source_files = [
            *[
                (path, path.relative_to(SHIP_DIR))
                for path in SHIP_DIR.rglob("*")
                if path.is_file()
            ],
            *self.RELEASE_ZIP_FILES,
        ]
        create_zip(zip_path, source_files)
        print(f"Created {zip_path}")
