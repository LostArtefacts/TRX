from shared.common import SRC_DIR
from subprocess import check_output


def get_branch_version(branch: str | None) -> str:
    return check_output(
        [
            "git",
            "describe",
            *([branch] if branch else ["--dirty"]),
            "--always",
            "--abbrev=7",
            "--tags",
            "--exclude",
            "latest",
        ],
        cwd=SRC_DIR,
        text=True,
    ).strip()


def generate_version() -> str:
    version = get_branch_version(None)
    return f'TR1X {version or "?"}'
