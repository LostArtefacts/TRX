# Coding guidelines

## Top values

- Compatibility with the original game's look and feel
- Player choice whether to enable any impactful changes
- Maintainability
- Automation where possible
- Documentation (git history and GitHub issues are great for this purpose)

## Automatic code formatting

This project uses [pre-commit](https://pre-commit.com/) to make sure the code
is formatted the right way. This tool has additional external dependencies:
`clang-format` for automatic code formatting. To install pre-commit:

```console
python3 -m pip install --user pre-commit
pre-commit install
```

To install required external dependencies on Ubuntu:

```console
apt-get install -y clang-format-22
```

After this, each commit should trigger a hook to automatically format changes.
To manually initiate this process, run `just lint-format`. This excludes the
slower checks that could affect productivity. For the full process, run `just
lint`. If installing the above software isn't possible, the CI pipeline will
indicate necessary changes in case of mistakes.

## Coding conventions

- Variables are `lower_snake_case`
- Global variables are `g_PascalCase`
- Module variables are `m_PascalCase` and static
- Global function names are `Module_PascalCase`
- Module functions are `M_PascalCase` and static
- Macros are `UPPER_SNAKE_CASE`
- Struct names are `UPPER_SNAKE_CASE`
- Struct members are `lower_snake_case`
- Enum names are `UPPER_SNAKE_CASE`
- Enum members are `UPPER_SNAKE_CASE`

It's recommended to minimize the use of global variables. Instead, consider
declaring them as `static` within the module they're used.

Other things:

- Within a file, top-level declarations follow the order: defines, local module
  types (typedef structs), static module variables, static module functions,
  public functions. Deviate only when code shape requires it (e.g. a forward
  declaration or a type dependency that forces a different order).
- We use clang-format to automatically format the code.
- We do not omit `{` and `}`.
- We use K&R brace style.
- We condense consecutive `if` expressions into one.

    Recommended:

    ```c
    if (a && b) {
    }
    ```

    Not recommended:

    ```c
    if (a) {
        if (b) {
        }
    }
    ```

    When expressions become extraordinarily complex, consider refactoring them
    into smaller conditions or functions.

## Naming numbers

A number that stands for something takes a suffix saying what kind it is.
`item.room_num` beside `item.room` is the shape.

- **`_num`** - needs a container to mean anything: `room_num` and `item_num`
  need the level, `anim_num` the object, `slot_num` the save pool.
- **`_id`** - reads on its own: `object_id`, `sample_id`, `track_id`, and a
  listener's `id`. Name the constants it takes: `type = "catalog.objects"`. An
  identity the engine mints is better handed over as a handle than as its
  number, and exposed for reading if at all.
- **A bare noun is the thing itself**, not a number for it: `item.room` is a
  handle. Quantities keep theirs as well - `damage`, `timer`, `volume`.
- **Counting follows the source**: from 0 where the level data numbered it, from
  1 where TRX made the list and the player sees the position.
- **`idx` stays in C**, for indices into its own arrays.

A number that several declarations hold is a type of its own: declare it once
with `api.number` and name it as their type. What it counts, and where it
counts from, is then written once, and what holds one says only what is its
own.

## Tooling

Internal tools are typically coded in a reasonably recent version of Python,
while avoiding the use of bash, shell, and similar languages.
