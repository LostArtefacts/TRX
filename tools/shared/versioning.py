import re
from pathlib import Path

from shared.git import Git


def generate_version(version: int, repo_dir: Path | None = None) -> str:
    git = Git(repo_dir=repo_dir)
    pattern: str
    if version == 1:
        pattern = "tr1-*"
    elif version == 2:
        pattern = "tr2-*"
    else:
        raise ValueError("Must supply game version")
    return (
        re.sub(
            "^tr[0-9]-",
            "",
            git.get_branch_version(pattern=pattern, branch=None),
        )
        or "?"
    )
