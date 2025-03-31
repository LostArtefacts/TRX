import argparse
import os
from dataclasses import dataclass, fields
from pathlib import Path
from subprocess import check_call, run
from typing import Any, Self

from shared.packaging import create_zip
from shared.versioning import generate_version


@dataclass
class BaseOptions:
    platform: str
    tr_version: int

    @property
    def build_root(self) -> Path:
        return Path(f"/app/build/tr{self.tr_version}/{self.platform}/")

    @property
    def version(self) -> str:
        return generate_version(self.tr_version)

    @classmethod
    def from_args(cls, args: argparse.Namespace) -> Self:
        cls_fields = [field.name for field in fields(cls)]
        filtered_args = vars(args)
        filtered_args = {
            k: v for k, v in filtered_args.items() if k in cls_fields
        }
        return cls(**filtered_args)


@dataclass
class PackageOptions(BaseOptions):
    @property
    def release_zip_filename(self) -> Path:
        platform = self.platform
        if platform == "win":
            platform = "windows"
        return Path(
            f"TR{self.tr_version}X-{self.version}-{platform.title()}.zip"
        )

    @property
    def ship_dir(self) -> Path:
        return Path(f"/app/data/tr{self.tr_version}/ship/")

    @property
    def release_zip_files(self) -> list[tuple[Path, str]]:
        if self.platform == "linux":
            return [
                (
                    self.build_root / f"TR{self.tr_version}X",
                    "TR{self.tr_version}X",
                )
            ]

        elif self.platform == "win":
            return [
                (
                    self.build_root / f"TR{self.tr_version}X.exe",
                    f"TR{self.tr_version}X.exe",
                ),
                (
                    Path(
                        f"/app/tools/tr{self.tr_version}/config/out/TR{self.tr_version}X_ConfigTool.exe"
                    ),
                    f"TR{self.tr_version}X_ConfigTool.exe",
                ),
            ]

        return []


@dataclass
class BuildOptions(BaseOptions):
    target: str

    strip_tool = "strip"
    upx_tool = "upx"

    @property
    def build_args(self) -> list[str]:
        if self.platform == "win":
            return [
                "--cross",
                "/app/tools/shared/docker/game-win/meson_linux_mingw32.txt",
            ]
        return []

    @property
    def compressable_exes(self) -> list[Path]:
        if self.platform == "linux":
            return [self.build_root / f"TR{self.tr_version}X"]
        elif self.platform == "win":
            return [self.build_root / f"TR{self.tr_version}X.exe"]
        return []

    @property
    def build_target(self) -> Path:
        return Path(f"src/tr{self.tr_version}")


def compress_exe(options: BuildOptions, path: Path) -> None:
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


class BuildCommand(BaseCommand):
    name = "build"

    def decorate_parser(self, parser: argparse.ArgumentParser) -> None:
        parser.add_argument("--platform")
        parser.add_argument("--tr-version", type=int, required=True)
        parser.add_argument(
            "--target",
            choices=["debug", "release", "debugoptim"],
            required=True,
        )

    def run(self, args: argparse.Namespace) -> None:
        options = BuildOptions.from_args(args)
        pkg_config_path = os.environ.get("PKG_CONFIG_PATH")

        if not (options.build_root / "build.ninja").exists():
            command: list[str | Path] = [
                "meson",
                "setup",
                "--buildtype",
                options.target,
                *options.build_args,
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
        parser.add_argument("--tr-version", type=int, required=True)
        parser.add_argument("-o", "--output", type=Path)

    def run(self, args: argparse.Namespace) -> None:
        options = PackageOptions.from_args(args)
        if args.output:
            zip_path = args.output
        else:
            zip_path = options.release_zip_filename

        source_files = [
            *[
                (path, str(path.relative_to(options.ship_dir)))
                for path in options.ship_dir.rglob("*")
                if path.is_file()
            ],
            *options.release_zip_files,
        ]

        create_zip(zip_path, source_files)
        print(f"Created {zip_path}")


def parse_args(
    commands: dict[str, BaseCommand], **kwargs
) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Docker entrypoint")
    subparsers = parser.add_subparsers(dest="action", help="Subcommands")
    parser.set_defaults(action="build", command=commands["build"])
    parser.set_defaults(**kwargs)

    for command in commands.values():
        subparser = subparsers.add_parser(command.name, help=command.help)
        command.decorate_parser(subparser)
        subparser.set_defaults(command=command)
        subparser.set_defaults(**kwargs)
    result = parser.parse_args()
    return result


def main(**kwargs: Any) -> None:
    commands = {
        command_cls.name: command_cls()
        for command_cls in BaseCommand.__subclasses__()
    }
    args = parse_args(commands, **kwargs)
    args.command.run(args)
