from pathlib import Path
import json
import pyjson5

SRC_DIR = Path(__file__).parent
REPO_DIR = SRC_DIR.parent
BUILD_DIR = REPO_DIR / "build"


def main() -> None:
    gf = pyjson5.loads(
        (REPO_DIR / "cfg/Tomb1Main_gameflow.json5").read_text(encoding="utf-8")
    )
    with (BUILD_DIR / "init.c").open("w") as handle:
        print('#include "init.h"', file=handle)
        print(file=handle)
        print('#include "game/vars.h"', file=handle)
        print(file=handle)
        print("void T1MInit()", file=handle)
        print("{", file=handle)
        for key, value in gf["strings"].items():
            print(
                f"    GF.strings[GS_{key}] = {json.dumps(value)};",
                file=handle,
            )
        print("}", file=handle)


if __name__ == "__main__":
    main()
