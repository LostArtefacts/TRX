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

TR1_DATA_DIR = DATA_DIR / "tr1"
TR1_DOCS_DIR = DOCS_DIR / "tr1"
TR1_SHIPPED_DATA_DIR = TR1_DATA_DIR / "ship"
TR1_SRC_DIR = SRC_DIR / "tr1"
TR1_TOOLS_DIR = TOOLS_DIR / "tr1"

TR2_DATA_DIR = DATA_DIR / "tr2"
TR2_SHIPPED_DATA_DIR = TR2_DATA_DIR / "ship"
TR2_SRC_DIR = SRC_DIR / "tr2"
TR2_TOOLS_DIR = TOOLS_DIR / "tr2"
TR2_DOCS_DIR = DOCS_DIR / "tr2"
TR2_PROGRESS_FILE = TR2_DOCS_DIR / 'progress.txt'
TR2_PROGRESS_SVG = TR2_DOCS_DIR / 'progress.svg'
