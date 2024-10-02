from pathlib import Path

REPO_DIR = Path(__file__).parent
while REPO_DIR.parent != REPO_DIR and not (REPO_DIR / ".git").exists():
    REPO_DIR = REPO_DIR.parent

TOOLS_DIR = REPO_DIR / "tools"
SRC_DIR = REPO_DIR / "src"
DATA_DIR = REPO_DIR / "data"
DOCS_DIR = REPO_DIR / "docs"

SHARED_INCLUDE_DIR = SRC_DIR / "libtrx/include/libtrx"
SHARED_SRC_DIR = SRC_DIR / "libtrx"


class ProjectPaths:
    def __init__(self, folder_name: str) -> None:
        self.folder_name = folder_name

        self.data_dir = DATA_DIR / folder_name
        self.shipped_data_dir = self.data_dir / "ship"
        self.src_dir = SRC_DIR / folder_name
        self.tools_dir = TOOLS_DIR / folder_name
        self.docs_dir = DOCS_DIR / folder_name
        self.changelog_path = self.docs_dir / "CHANGELOG.md"


TR1Paths = ProjectPaths(folder_name="tr1")

TR2Paths = ProjectPaths(folder_name="tr2")
TR2Paths.progress_file = TR2Paths.docs_dir / "progress.txt"
TR2Paths.progress_svg = TR2Paths.docs_dir / "progress.svg"

PROJECT_PATHS = {1: TR1Paths, 2: TR2Paths}
