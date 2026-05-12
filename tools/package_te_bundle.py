#!/usr/bin/env python3

from __future__ import annotations

import argparse
import shutil
import tempfile
import zipfile
from pathlib import Path

from shared.packaging import create_zip
from shared.versioning import generate_version


REPO_DIR = Path(__file__).resolve().parent.parent
DATA_TE_DIR = REPO_DIR / "data" / "te"
TE_ENGINES: tuple[str, ...] = ("tr1", "tr2", "tr3")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--artifact", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument(
        "--asset-suffix",
        default="TombEditor",
        type=str,
    )
    parser.add_argument("--no-zip", action="store_true")
    return parser.parse_args()


def iter_te_files() -> list[tuple[Path, str]]:
    source_files: list[tuple[Path, str]] = []
    for path in sorted(DATA_TE_DIR.rglob("*")):
        if path.is_dir():
            continue

        rel_path = path.relative_to(DATA_TE_DIR)
        if path.is_symlink():
            real_path = path.resolve(strict=True)
            source_files.append((real_path, str(rel_path)))
        else:
            source_files.append((path, str(rel_path)))
    return source_files


def iter_exe_files(exe_path: Path) -> list[tuple[Path, str]]:
    return [
        (exe_path, f"{engine}/Engine/TRX.exe")
        for engine in TE_ENGINES
    ]


def resolve_exe_from_artifact(artifact_path: Path, temp_root: Path) -> Path:
    search_root = artifact_path
    if artifact_path.is_file():
        unpack_dir = temp_root / "windows_artifact"
        unpack_dir.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(artifact_path) as handle:
            handle.extractall(unpack_dir)
        search_root = unpack_dir

    for path in sorted(search_root.rglob("TRX.exe")):
        if path.is_file():
            return path
    raise FileNotFoundError(
        f"TRX.exe not found in windows artifact: {artifact_path}"
    )


def create_zip_from_dir(source_dir: Path, output_path: Path) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    source_files = [
        (path, str(path.relative_to(source_dir)))
        for path in sorted(source_dir.rglob("*"))
        if path.is_file()
    ]
    create_zip(output_path, source_files, compresslevel=9)


def main() -> None:
    args = parse_args()
    version = generate_version()
    output_stem = f"TRX-{version}-{args.asset_suffix}"
    output_path = args.output

    if output_path.suffix.lower() != ".zip" and not args.no_zip:
        output_path = output_path / (
            f"{args.artifact.name}.zip"
            if args.artifact.is_dir()
            else f"{output_stem}.zip"
        )
    elif args.no_zip and output_path.suffix.lower() == ".zip":
        raise ValueError("--no-zip output must be a directory path")

    with tempfile.TemporaryDirectory(prefix="trx-te-bundle-") as tmpdir:
        tmpdir_path = Path(tmpdir)
        exe_path = resolve_exe_from_artifact(args.artifact.resolve(), tmpdir_path)

        source_files = [*iter_te_files(), *iter_exe_files(exe_path)]
        tmp_root = tmpdir_path / "bundle"
        tmp_root.mkdir(parents=True, exist_ok=True)

        for src_path, rel_path in source_files:
            dst_path = tmp_root / rel_path
            dst_path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy(src_path, dst_path)

        if args.no_zip:
            output_path.mkdir(parents=True, exist_ok=True)
            for path in tmp_root.iterdir():
                dst_path = output_path / path.name
                if dst_path.exists():
                    if dst_path.is_dir():
                        shutil.rmtree(dst_path)
                    else:
                        dst_path.unlink()
                if path.is_dir():
                    shutil.copytree(path, dst_path)
                else:
                    shutil.copy(path, dst_path)
            print(f"Created {output_path}")
        else:
            output_path.parent.mkdir(parents=True, exist_ok=True)
            create_zip_from_dir(tmp_root, output_path)
            print(f"Created {output_path}")


if __name__ == "__main__":
    main()
