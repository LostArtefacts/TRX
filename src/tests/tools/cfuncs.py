#!/usr/bin/env python3

"""The C parser the declaration-order checks read a source through.

It finds a definition by its opening brace and walks back over the signature.
What tells it where the signature starts is the shape of the line above, so a
definition written straight under the closing brace of the one before it is
the case to hold: read as a continuation, it reports the earlier definition a
second time and the check never sees the later one at all.
"""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools"))

from shared.cfuncs import definitions, is_static, name_of  # noqa: E402

CASES = [
    (
        "a definition under the closing brace of the one before it",
        "void Public_One(void)\n{\n}\nstatic void M_After(void)\n{\n}\n",
        [(1, "Public_One"), (4, "M_After")],
    ),
    (
        "a blank line between them",
        "void Public_One(void)\n{\n}\n\nstatic void M_After(void)\n{\n}\n",
        [(1, "Public_One"), (5, "M_After")],
    ),
    (
        "a signature spanning several lines",
        "void A(void)\n{\n}\nstatic void M_B(\n    const int x,\n    const int y)\n{\n}\n",
        [(1, "A"), (4, "M_B")],
    ),
    (
        "a comment sitting on the definition",
        "void A(void)\n{\n}\n// why\nstatic void M_B(void)\n{\n}\n",
        [(1, "A"), (5, "M_B")],
    ),
    (
        "a preprocessor line above",
        "#ifdef _WIN32\nstatic void M_B(void)\n{\n}\n#endif\n",
        [(2, "M_B")],
    ),
    (
        "an initialiser, which keeps its brace on the line that opens it",
        "static const int m_Table[] = {\n    1,\n};\nstatic void M_B(void)\n{\n}\n",
        [(4, "M_B")],
    ),
]


def main() -> int:
    failures = []

    for label, source, expected in CASES:
        found = [(line, name_of(sig)) for line, sig in definitions(source)]
        if found != expected:
            failures.append(f"{label}: expected {expected}, got {found}")

    static_source = "void A(void)\n{\n}\nstatic void M_B(void)\n{\n}\n"
    staticness = [is_static(sig) for _, sig in definitions(static_source)]
    if staticness != [False, True]:
        failures.append(f"is_static: expected [False, True], got {staticness}")

    for failure in failures:
        print(f"  FAIL  {failure}", file=sys.stderr)
    if failures:
        return 1

    print(f"  PASS  {len(CASES) + 1} cases")
    return 0


if __name__ == "__main__":
    sys.exit(main())
