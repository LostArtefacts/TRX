import argparse
import os
from dataclasses import dataclass
from pathlib import Path
from subprocess import check_call, run
from typing import Any

from shared.packaging import create_zip
from shared.versioning import generate_version


@dataclass
class Options:
    version: int
    platform: str
    compile_args: list[str]
    release_zip_files: list[tuple[Path, str]]
    strip_tool = "strip"
    upx_tool = "upx"
    target = os.environ.get("TARGET", "debug")
    compressable_exes: list[Path] | None = None

    @property
    def ship_dir(self) -> Path:
        return Path(f"/app/data/tr{self.version}/ship/")

    @property
    def build_root(self) -> Path:
        return Path(f"/app/build/tr{self.version}/{self.platform}/")

    @property
    def build_target(self) -> str:
        return f"src/tr{self.version}"

    @property
    def release_zip_filename_fmt(self) -> str:
        platform = self.platform
        if platform == "win":
            platform = "windows"
        return f"TR{self.version}X-{{version}}-{platform.title()}.zip"


def compress_exe(options: Options, path: Path) -> None:
    if run([options.upx_tool, "-t", str(path)]).returncode != 0:
        check_call([options.strip_tool, str(path)])
        check_call([options.upx_tool, str(path)])


class BaseCommand:
    name: str = NotImplemented
    help: str = NotImplemented

    def decorate_parser(self, parser: argparse.ArgumentParser) -> None:
        pass

    def run(self, args: argparse.Namespace) -> None:
        raise NotImplementedError("not implemented")


class CompileCommand(BaseCommand):
    name = "compile"

    def run(self, args: argparse.Namespace, options: Options) -> None:
        pkg_config_path = os.environ.get("PKG_CONFIG_PATH")

        if not (options.build_root / "build.ninja").exists():
            command = [
                "meson",
                "setup",
                "--buildtype",
                options.target,
                *options.compile_args,
                options.build_root,
                options.build_target,
            ]
            if pkg_config_path:
                command.extend(["--pkg-config-path", pkg_config_path])
            check_call(command)

        check_call(["meson", "compile"], cwd=options.build_root)

        if options.target == "release":
            for exe_path in options.compressable_exes:
                compress_exe(options, exe_path)


class PackageCommand(BaseCommand):
    name = "package"

    def decorate_parser(self, parser: argparse.ArgumentParser) -> None:
        parser.add_argument("-o", "--output", type=Path)

    def run(self, args: argparse.Namespace, options: Options) -> None:
        if args.output:
            zip_path = args.output
        else:
            zip_path = Path(
                options.release_zip_filename_fmt.format(
                    version=generate_version(options.version)
                )
            )

        source_files = [
            *[
                (path, path.relative_to(options.ship_dir))
                for path in options.ship_dir.rglob("*")
                if path.is_file()
            ],
            *options.release_zip_files,
        ]

        create_zip(zip_path, source_files)
        print(f"Created {zip_path}")


def parse_args(commands: dict[str, BaseCommand]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Docker entrypoint")
    subparsers = parser.add_subparsers(dest="action", help="Subcommands")
    parser.set_defaults(action="compile", command=commands["compile"])

    for command in commands.values():
        subparser = subparsers.add_parser(command.name, help=command.help)
        command.decorate_parser(subparser)
        subparser.set_defaults(command=command)
    result = parser.parse_args()
    # if not hasattr(result, "command"):
    #     args.action = "compile"
    #     args.command = CompileCommand
    return result


def run_script(**kwargs: Any) -> None:
    commands = {
        command_cls.name: command_cls()
        for command_cls in BaseCommand.__subclasses__()
    }
    args = parse_args(commands)
    options = Options(**kwargs)
    args.command.run(args, options)
