import json
import subprocess
from pathlib import Path

TOMBSTONES_PATH = "cfg/tombstones.json5"

FNV1A64_BASE = 14695981039346656037
FNV1A64_PRIME = 1099511628211
U64_MASK = (1 << 64) - 1
NULL_BLOB = "0" * 40


def fnv1a64(data: bytes) -> int:
    value = FNV1A64_BASE
    for byte in data:
        value = ((value ^ byte) * FNV1A64_PRIME) & U64_MASK
    return value


def _git(repo_dir: Path, *args: str, stdin: bytes | None = None) -> bytes:
    return subprocess.run(
        ["git", *args],
        cwd=repo_dir,
        input=stdin,
        capture_output=True,
        check=True,
    ).stdout


def _collect_blobs(repo_dir: Path, ship_dir: Path) -> dict[str, set[str]]:
    output = _git(
        repo_dir,
        "log",
        "-m",
        "--no-renames",
        "--raw",
        "--no-abbrev",
        "--format=",
        "-z",
        "--",
        str(ship_dir.relative_to(repo_dir)),
    )
    blobs: dict[str, set[str]] = {}
    fields = output.split(b"\0")
    for meta, path in zip(fields[0::2], fields[1::2]):
        _old_mode, _new_mode, old_blob, new_blob, _status = (
            meta.decode().lstrip(":").split()
        )
        rel_path = (
            (repo_dir / path.decode()).relative_to(ship_dir).as_posix()
        )
        blobs.setdefault(rel_path, set()).update(
            blob for blob in (old_blob, new_blob) if blob != NULL_BLOB
        )
    return blobs


def _read_blobs(repo_dir: Path, blob_ids: list[str]) -> dict[str, bytes]:
    output = _git(
        repo_dir,
        "cat-file",
        "--batch",
        stdin="".join(f"{blob_id}\n" for blob_id in blob_ids).encode(),
    )
    contents: dict[str, bytes] = {}
    pos = 0
    for blob_id in blob_ids:
        header_end = output.index(b"\n", pos)
        size = int(output[pos:header_end].split()[2])
        start = header_end + 1
        contents[blob_id] = output[start : start + size]
        pos = start + size + 1
    return contents


def generate_tombstones(repo_dir: Path, ship_dir: Path) -> dict[str, list[str]]:
    if _git(repo_dir, "rev-parse", "--is-shallow-repository").strip() == b"true":
        raise RuntimeError(
            "Tombstones need the full git history; fetch with --unshallow"
        )

    shipped = {
        path.relative_to(ship_dir).as_posix()
        for path in ship_dir.rglob("*")
        if not path.is_dir()
    }
    removed = {
        rel_path: blob_ids
        for rel_path, blob_ids in _collect_blobs(repo_dir, ship_dir).items()
        if rel_path not in shipped
    }
    contents = _read_blobs(
        repo_dir, sorted({blob for blobs in removed.values() for blob in blobs})
    )
    return {
        rel_path: sorted(
            {f"{fnv1a64(contents[blob_id]):016x}" for blob_id in blob_ids}
        )
        for rel_path, blob_ids in sorted(removed.items())
    }


def write_tombstones(
    repo_dir: Path, ship_dir: Path, output_path: Path
) -> None:
    tombstones = generate_tombstones(repo_dir, ship_dir)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(tombstones, indent=4) + "\n")
