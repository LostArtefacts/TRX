import re
from dataclasses import dataclass
from datetime import date
from pathlib import Path

from dateutil.parser import parse as parse_date

BASE_URL = "https://github.com/LostArtefacts/TRX"


@dataclass
class Changelog:
    release_name: str
    body: str
    commit_tag: str
    diff_url: str
    release_date: date | None

    @property
    def commits_url(self):
        return f"{BASE_URL}/commits/{self.commit_tag}/"


def get_current_version_changelog(changelog_path: Path) -> Changelog:
    sections = [
        section.strip()
        for section in re.split(
            "^(?=##)", changelog_path.read_text(), flags=re.M
        )
        if re.search(r"- \w", section)
    ]
    if sections:
        section_lines = sections[0].splitlines()
        header = section_lines.pop(0)
        return Changelog(
            release_name=re.search(r"## \[([^\]]+?)\]", header).group(1),
            commit_tag=re.search(r"\.\.\.([^\)]+?)\)", header).group(1),
            diff_url=re.search(r"\(([^\)]+?)\)", header).group(1),
            release_date=(
                parse_date(match.group(1))
                if (match := re.search(r" - (\d{4}-\d{2}-\d{2})", header))
                else None
            ),
            body="\n".join(
                line for line in section_lines if not line.startswith("#")
            ),
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
    today = date.today().strftime("%Y-%m-%d")
    repo_url = "https://github.com/LostArtefacts/TRX"
    changelog = re.sub(r"^## \[Unreleased\].*\n*", "", changelog, flags=re.M)
    changelog = (
        f"## [Unreleased]({repo_url}/compare/{new_tag}...{develop_branch}) - ××××-××-××\n\n"
        f"## [{new_version_name}]({repo_url}/compare/{old_tag}...{new_tag}) - {today}\n"
        + changelog
    )
    return changelog
