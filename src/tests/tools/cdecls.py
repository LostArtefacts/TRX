#!/usr/bin/env python3

"""The declaration-order check against the shapes it has to tell apart.

docs/CODING_GUIDELINES.md orders a file defines, types, static variables,
static functions, public functions. What the check has to get right is which
of the five a line is: a define continued with a backslash, a brace inside a
string, and anything written inside a function body are all places a naive
read finds a declaration that is not there.
"""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools"))

from shared.cdecls import declarations, out_of_order  # noqa: E402

ORDERED = """\
#define M_LIMIT 4

typedef struct {
    int x;
} M_THING;

static M_THING m_Thing = { .x = 1 };

static void M_Helper(void)
{
}

void Module_Public(void)
{
}
"""

CASES = [
    (
        "a file in order",
        ORDERED,
        ["define", "type", "static variable", "static function", "public function"],
    ),
    (
        "a define continued across lines",
        "#define M_WIDE(a, b) \\\n    ((a) + (b))\nstatic int m_N = 0;\n",
        ["define", "static variable"],
    ),
    (
        "declarations written inside a function body",
        "void Module_Public(void)\n{\n#define M_INNER 1\n    static int m_Cache;\n}\n",
        ["public function"],
    ),
    (
        "a brace inside a string literal",
        'static const char *m_Fmt = "{";\nstatic void M_Helper(void)\n{\n}\n',
        ["static variable", "static function"],
    ),
    (
        "an array initialiser spanning lines",
        "static const int m_Table[] = {\n    1,\n    2,\n};\nvoid Module_Public(void)\n{\n}\n",
        ["static variable", "public function"],
    ),
]


def kinds(source):
    return [decl.kind for decl in declarations(source)]


def bad_lines(source):
    return [decl.line for decl, _ in out_of_order(source)]


def main() -> int:
    failures = []

    for label, source, expected in CASES:
        found = kinds(source)
        if found != expected:
            failures.append(f"{label}: expected {expected}, got {found}")

    if bad_lines(ORDERED) != []:
        failures.append("a file in order was reported out of order")

    # Each of the four ways to sit below something that outranks you.
    for label, source, expected_lines in [
        ("a define under a static function", "static void M_A(void)\n{\n}\n#define M_L 1\n", [4]),
        ("a type under a static variable", "static int m_N;\ntypedef struct {\n    int x;\n} M_T;\n", [2]),
        ("a static variable under a static function", "static void M_A(void)\n{\n}\nstatic int m_N;\n", [4]),
        ("a static function under a public one", "void Module_P(void)\n{\n}\nstatic void M_A(void)\n{\n}\n", [4]),
    ]:
        found = bad_lines(source)
        if found != expected_lines:
            failures.append(f"{label}: expected {expected_lines}, got {found}")

    # A table naming the static functions above it has to sit below them, or
    # every one of them needs a forward declaration first.
    reaching_back = (
        "static void M_Step(void)\n{\n}\n\n"
        "static const MODULE m_Module = {\n    .step = M_Step,\n};\n"
    )
    if bad_lines(reaching_back) != []:
        failures.append("a table reaching back to a static function was reported")

    # The same shape naming nothing above it has nothing forcing it down.
    reaching_nothing = (
        "static void M_Step(void)\n{\n}\n\nstatic const int m_Limit = 4;\n"
    )
    if bad_lines(reaching_nothing) != [5]:
        failures.append(
            f"a plain static below a function: expected [5], "
            f"got {bad_lines(reaching_nothing)}"
        )

    # A define the file takes back with #undef reaches only the lines between
    # the two, and what sits there is written expecting it.
    scoped = (
        "static void M_A(void)\n{\n}\n\n"
        "#define X_ENTRY(name) name,\n"
        "#include <trx/thing.def>\n"
        "#undef X_ENTRY\n"
    )
    if bad_lines(scoped) != []:
        failures.append("an #undef'd define was told to move out of its stretch")

    # A forward declaration names no storage, and the guidelines give one as a
    # reason to deviate. A pointer to a function does name storage.
    forwards = "static void M_A(void)\n{\n}\n\nstatic bool M_B(const char *s);\n"
    if [d.kind for d in declarations(forwards)] != ["static function"]:
        failures.append("a forward declaration was read as a declaration")
    pointer = "static void M_A(void)\n{\n}\n\nstatic void (*m_Handlers[4])(int);\n"
    if bad_lines(pointer) != [5]:
        failures.append(
            f"a function-pointer variable: expected [5], got {bad_lines(pointer)}"
        )

    for failure in failures:
        print(f"  FAIL  {failure}", file=sys.stderr)
    if failures:
        return 1

    print(f"  PASS  {len(CASES) + 10} cases")
    return 0


if __name__ == "__main__":
    sys.exit(main())
