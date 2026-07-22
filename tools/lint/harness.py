import sys
from collections.abc import Callable, Iterable
from dataclasses import dataclass
from pathlib import Path

# Checks import shared helpers (json_utils, paths) as `shared.*`; put the tools
# directory on the path so those resolve from anywhere under tools/lint.
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))


@dataclass
class LintWarning:
    path: Path
    message: str
    line: int | None = None

    def __str__(self) -> str:
        prefix = str(self.path)
        if self.line is not None:
            prefix += f":{self.line}"
        return f"{prefix}: {self.message}"


def _report(warnings: Iterable[LintWarning]) -> int:
    exit_code = 0
    for warning in warnings:
        print(str(warning), file=sys.stderr)
        exit_code = 1
    return exit_code


def file_check(
    check: Callable[[Path, str], Iterable[LintWarning]],
    fix: Callable[[Path, str], str] | None = None,
) -> None:
    # Run `check` over each file prek passes on the command line. Files that do
    # not decode as UTF-8 are binary and get skipped; the hook's `files` and
    # `exclude` patterns handle everything else. Exits non-zero on any warning.
    #
    # With `--fix` and a `fix` rewriter, apply it in place and report nothing;
    # the check still runs on the rewritten text so anything `fix` cannot
    # resolve is surfaced as a warning.
    apply_fix = fix is not None and "--fix" in sys.argv[1:]
    warnings: list[LintWarning] = []
    for arg in sys.argv[1:]:
        if arg.startswith("-"):
            continue
        path = Path(arg)
        if not path.is_file():
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        if apply_fix:
            fixed = fix(path, text)
            if fixed != text:
                path.write_text(fixed, encoding="utf-8")
            text = fixed
        warnings.extend(check(path, text))
    sys.exit(_report(warnings))


def repo_check(check: Callable[[], Iterable[LintWarning]]) -> None:
    # Run a check that finds its own inputs rather than taking file arguments.
    # Registered with `pass_filenames: false` and gated on its inputs.
    sys.exit(_report(check()))
