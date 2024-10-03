import re
from datetime import datetime
from pathlib import Path


def get_current_version_changelog(changelog_path: Path) -> str:
    sections = [
        section
        for section in changelog_path.read_text().split("\n\n")
        if re.search(r"- \w", section)
    ]
    if sections:
        section = sections[0]
        return "\n".join(
            line for line in section.splitlines() if not line.startswith("#")
        )


def update_changelog_to_new_version(
    changelog: str,
    old_tag: str,
    new_tag: str,
    new_version_name: str,
    stable_branch: str | None = "stable",
    develop_branch: str = "develop",
) -> str:
    if f"[{new_version_name}]" in changelog:
        return changelog
    repo_url = 'https://github.com/LostArtefacts/TRX'
    changelog = re.sub('^## \[Unreleased\].*\n*', '', changelog, flags=re.M)
    changelog = (
        f"## [Unreleased]({repo_url}/compare/{new_tag}...{develop_branch}) - ××××-××-××\n\n"
        f"## [{new_version_name}]({repo_url}/compare/{old_tag}...{new_tag}) - {datetime.now().strftime("%Y-%m-%d")}\n"
        + changelog
    )
    return changelog
