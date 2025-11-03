import re
from pathlib import Path

from shared.git import Git


def generate_version(repo_dir: Path | None = None) -> str:
    git = Git(repo_dir=repo_dir)
    return (
        re.sub(
            "^trx-",
            "",
            git.get_branch_version(pattern="trx-*", branch=None),
        )
        or "?"
    )


def generate_package_name(
    engine_version: str, platform: str, game_version: int
) -> str:
    if platform == "win":
        platform = "windows"
    elif platform == "win-installer":
        platform = "windows_installer"
    platform = platform.title()
    return f"TRX-{engine_version}-{platform}-tr{game_version}"
