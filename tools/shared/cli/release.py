import argparse
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from shared.changelog import update_changelog_to_new_version
from shared.git import Git


@dataclass
class Options:
    project_name: str
    changelog_path: Path
    stable_branch: str = "stable"
    develop_branch: str = "develop"


class BaseCommand:
    name: str = NotImplemented
    help: str = NotImplemented

    def __init__(self, git: Git) -> None:
        self.git = git

    def decorate_parser(self, parser: argparse.ArgumentParser) -> None:
        parser.add_argument("version")

    def run(self, args: argparse.Namespace, options: Options) -> None:
        raise NotImplementedError("not implemented")


class CommitCommand(BaseCommand):
    name = "commit"
    help = "Create and tag a commit with the release information"

    def decorate_parser(self, parser: argparse.ArgumentParser) -> None:
        super().decorate_parser(parser)
        parser.add_argument(
            "-d",
            "--dry-run",
            action="store_true",
            help="only output the changelog to stdout, do not commit anything",
        )

    def run(self, args: argparse.Namespace, options: Options) -> None:
        # self.git.checkout_branch("develop")
        old_version = self.git.get_branch_version("origin/stable")
        new_version = args.version

        old_changelog = options.changelog_path.read_text()
        new_changelog = update_changelog_to_new_version(
            old_changelog,
            old_version,
            new_version,
            project_name=options.project_name,
            stable_branch=options.stable_branch,
            develop_branch=options.develop_branch,
        )
        if old_changelog == new_changelog:
            return

        if args.dry_run:
            print(new_changelog, end="")
            return

        options.changelog_path.write_text(new_changelog)
        self.git.add(options.changelog_path)
        self.git.commit(f"docs: release {args.version}")
        self.git.delete_tag(args.version)
        self.git.create_tag(args.version)


class BranchCommand(BaseCommand):
    name = "branch"
    help = "Merge branch to the specified tag"

    def run(self, args: argparse.Namespace, options: Options) -> None:
        self.git.checkout_branch("stable")
        self.git.reset(args.version, hard=True)
        self.git.checkout_branch("develop")


class PushCommand(BaseCommand):
    name = "push"
    help = (
        "Push the develop and stable branches, and the version tag to GitHub"
    )

    def run(self, args, options: Options) -> None:
        self.git.push(
            "origin", ["develop", "stable", args.version], force=True
        )


def parse_args(commands: list[BaseCommand]) -> None:
    parser = argparse.ArgumentParser(
        description="Argument parser with subcommands"
    )
    subparsers = parser.add_subparsers(title="subcommands", dest="subcommand")
    for command in commands:
        subparser = subparsers.add_parser(command.name, help=command.help)
        command.decorate_parser(subparser)
        subparser.set_defaults(command=command)

    result = parser.parse_args()
    if not hasattr(result, "command"):
        parser.error("missing command")
    return result


def run_script(**kwargs: Any) -> None:
    git = Git()
    commands = [
        command_cls(git=git) for command_cls in BaseCommand.__subclasses__()
    ]
    options = Options(**kwargs)
    args = parse_args(commands)
    args.command.run(args, options)
