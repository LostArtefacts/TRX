from subprocess import run


def generate_version() -> str:
    cmd = [
        "git",
        "describe",
        "--always",
        "--abbrev=7",
        "--tags",
        "--exclude",
        "latest",
    ]
    version = run(cmd, capture_output=True, text=True).stdout.strip()
    return f'TR1X {version or "?"}'
