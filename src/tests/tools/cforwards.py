#!/usr/bin/env python3

"""The forward-declaration check against the shapes it has to tell apart.

A forward declaration of a static function is needed only where the file names
the function before its definition. The check has to find such a use in a
call, in a table and in a macro body, and has to read a declaration written
through a macro as one of the function it names.
"""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools"))

from shared.cforwards import unneeded  # noqa: E402

CASES = [
    (
        "a declaration with no use before the definition",
        "static void M_A(int x);\n\nstatic void M_A(const int x)\n{\n}\n",
        [(1, "M_A")],
    ),
    (
        "a declaration spanning several lines",
        "static void M_A(\n    int x, int y);\n\n"
        "static void M_A(const int x, const int y)\n{\n}\n",
        [(1, "M_A")],
    ),
    (
        "a name that only a comment and a string mention",
        "static void M_A(void);\n\n"
        "// M_A\nstatic const char *m_Name = \"M_A\";\n\n"
        "static void M_A(void)\n{\n}\n",
        [(1, "M_A")],
    ),
    (
        "two functions that call each other",
        "static void M_B(int n);\n\n"
        "static void M_A(const int n)\n{\n    M_B(n - 1);\n}\n\n"
        "static void M_B(const int n)\n{\n    M_A(n - 1);\n}\n",
        [],
    ),
    (
        "a table of handlers above the functions",
        "static void M_Step(void);\n\n"
        "static const MODULE m_Module = {\n    .step = M_Step,\n};\n\n"
        "static void M_Step(void)\n{\n}\n",
        [],
    ),
    (
        "a declaration written through a macro",
        "#define M_DECLARE_HANDLER(name) int32_t name(void *ctx)\n\n"
        "static M_DECLARE_HANDLER(M_HandleA);\n"
        "static M_DECLARE_HANDLER(M_HandleB);\n\n"
        "static void *m_Handlers[] = { M_HandleA };\n\n"
        "static M_DECLARE_HANDLER(M_HandleA)\n{\n    return 0;\n}\n\n"
        "static M_DECLARE_HANDLER(M_HandleB)\n{\n    return 0;\n}\n",
        [(4, "M_HandleB")],
    ),
    (
        "a use the file does not spell out, kept by the marker",
        "// lint: keep forward declaration\nstatic void M_HandleA(void);\n\n"
        "M_TABLE_ENTRY(A)\n\nstatic void M_HandleA(void)\n{\n}\n",
        [],
    ),
    (
        "a pointer to a function, which is storage",
        "static void (*m_Hook)(int);\n\nstatic void M_A(void)\n{\n}\n",
        [],
    ),
    (
        "a declaration with no definition in the file",
        "static void M_A(void);\n",
        [],
    ),
    (
        "a declaration inside a function body",
        "void Module_P(void)\n{\n    static int m_N;\n}\n",
        [],
    ),
]


def main() -> int:
    failures = []

    for label, source, expected in CASES:
        found = [(forward.line, forward.name) for forward in unneeded(source)]
        if found != expected:
            failures.append(f"{label}: expected {expected}, got {found}")

    for failure in failures:
        print(f"  FAIL  {failure}", file=sys.stderr)
    if failures:
        return 1

    print(f"  PASS  {len(CASES)} cases")
    return 0


if __name__ == "__main__":
    sys.exit(main())
