import re
from pathlib import Path

from shared.git import Git

PATTERN_MAP = {
    1: "tr1-*",
    2: "tr2-*",
}


def generate_version(version: int, repo_dir: Path | None = None) -> str:
    git = Git(repo_dir=repo_dir)
    pattern = PATTERN_MAP[version]
    return (
        re.sub(
            "^tr[0-9]-",
            "",
            git.get_branch_version(pattern=pattern, branch=None),
        )
        or "?"
    )
