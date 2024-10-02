from pathlib import Path

TR1X_TOOLS_DIR = Path(__file__).parent.parent / "tr1"
TR1X_REPO_DIR = TR1X_TOOLS_DIR.parent
TR1X_DATA_DIR = TR1X_REPO_DIR / "data/tr1"
TR1X_SRC_DIR = TR1X_REPO_DIR / "src/tr1"

LIBTRX_REPO_DIR = TR1X_REPO_DIR / "subprojects/libtrx"
LIBTRX_SRC_DIR = LIBTRX_REPO_DIR / "src"
LIBTRX_INCLUDE_DIR = LIBTRX_REPO_DIR / "include/libtrx"
