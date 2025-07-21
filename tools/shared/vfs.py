import difflib
from pathlib import Path


class VirtualFilesystem:
    """Lazy disk operations to avoid disk thrashing."""

    def __init__(self) -> None:
        self.files = {}

    def get(self, path: Path) -> None:
        if content := self.files.get(path):
            return content
        content = path.read_text()
        self.files[path] = content
        return content

    def put(self, path: Path, content: str) -> None:
        self.files[path] = content

    def show_diff(self) -> None:
        for path, new_content in self.files.items():
            old_content = path.read_text()
            if old_content != new_content:
                print(
                    "".join(
                        difflib.unified_diff(
                            old_content.splitlines(keepends=True),
                            new_content.splitlines(keepends=True),
                            fromfile=str(path),
                            tofile=str(path),
                        )
                    )
                )

    def commit(self) -> None:
        for path, content in self.files.items():
            if path.read_text() != content:
                path.write_text(content)
