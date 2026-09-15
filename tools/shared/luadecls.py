import re

# A top-level chunk begins at a line in column zero and runs to the next one.
# Anything indented belongs to the chunk above it.
_FUNCTION = re.compile(r"^local function\b")
_CALL = re.compile(r"^[A-Za-z_][\w.\[\]\"':]*\s*[({\"]")
_CALLBACK = re.compile(r"function\s*\(")

_LONG_STRING = re.compile(r"\[(=*)\[.*?\]\1\]", re.DOTALL)
_QUOTED = re.compile(r"""(['"])(?:\\.|(?!\1).)*\1""", re.DOTALL)
_COMMENT = re.compile(r"--[^\n]*")


def _without_text(chunk: str) -> str:
    # Strings and comments hold code that never runs. A doc example showing
    # `trx.events.on_tick(function() ... end)` is text, not a registration.
    chunk = _LONG_STRING.sub(" ", chunk)
    chunk = _QUOTED.sub('""', chunk)
    return _COMMENT.sub("", chunk)


def _without_tables(chunk: str) -> str:
    # Only what the call itself takes counts. A function inside a table is a
    # field of a specification, as `api.define` and `trx.objects.declare` read
    # them, and the call hands over data rather than behavior.
    kept, depth = [], 0
    for char in chunk:
        if char == "{":
            depth += 1
        elif char == "}":
            depth = max(0, depth - 1)
        elif depth == 0:
            kept.append(char)
    return "".join(kept)


def _chunks(text: str):
    lines = text.splitlines()
    starts = [
        num
        for num, line in enumerate(lines)
        if line[:1] not in (" ", "\t", "") and not line.startswith("--")
    ]
    for i, num in enumerate(starts):
        end = starts[i + 1] if i + 1 < len(starts) else len(lines)
        yield num + 1, lines[num], "\n".join(lines[num:end])


def registers_callback(chunk: str) -> bool:
    # Whether a top-level chunk hands a function to a call, which is how a
    # script asks the engine to run something later.
    return bool(_CALLBACK.search(_without_tables(_without_text(chunk))))


def late_functions(text: str):
    # Each top-level `local function` that a callback registration already
    # stands above, as (line, name, registration line, registration).
    standing = None
    for line, head, chunk in _chunks(text):
        if _FUNCTION.match(head):
            if standing is not None:
                yield line, head.strip(), standing[0], standing[1].strip()
        elif _CALL.match(head) and registers_callback(chunk):
            standing = (line, head)
