from pathlib import Path
from subprocess import check_output, run


class Git:
    def __init__(self, repo_dir: Path | None = None) -> None:
        self.repo_dir = repo_dir

    def checkout_branch(self, branch_name: str) -> None:
        if self.check_output(["git", "diff", "--cached", "--name-only"]):
            raise RuntimeError("Staged files")
        self.check_output(["git", "checkout", branch_name])

    def reset(self, target: str, hard: bool = False) -> None:
        self.check_output(
            ["git", "reset", "develop", *(["--hard"] if hard else [])]
        )

    def delete_tag(self, tag_name: str) -> None:
        self.grab_output(["git", "tag", "-d", tag_name])

    def create_tag(self, tag_name: str) -> None:
        self.check_output(["git", "tag", tag_name])

    def add(self, target: str) -> None:
        self.check_output(["git", "add", target])

    def commit(self, message: str) -> None:
        self.check_output(["git", "commit", "-m", message])

    def push(
        self,
        upstream: str,
        targets: list[str],
        force_with_lease: bool = False,
        force: bool = False,
    ) -> None:
        self.check_output(
            [
                "git",
                "push",
                upstream,
                *targets,
                *(["--force-with-lease"] if force else []),
                *(["--force"] if force else []),
            ]
        )

    def get_current_commit_hash(self) -> str:
        return self.grab_output(
            ["git", "log", "-1", "--pretty=format:%H"]
        ).stdout.strip()

    def get_branch_version(
        self, pattern: str | None = None, branch: str | None = None
    ) -> str:
        return self.grab_output(
            [
                "git",
                "describe",
                *([branch] if branch else ["--dirty"]),
                "--always",
                "--abbrev=7",
                "--tags",
                "--exclude",
                "latest",
                *(["--match", pattern] if pattern else []),
            ]
        ).stdout.strip()

    def grab_output(self, *args, **kwargs):
        return run(
            *args, **kwargs, capture_output=True, text=True, cwd=self.repo_dir
        )

    def check_output(self, *args, **kwargs):
        return check_output(*args, **kwargs, cwd=self.repo_dir)
