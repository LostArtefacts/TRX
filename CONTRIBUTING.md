# Development guidelines

## Build workflow

Initial build:

- Compile the project (described in the next section)
- Copy all .dll and .exe files from `build/` to your game directory
- Copy the contents of `bin/` to your game directory

Subsequent builds:

- Compile the project
- Copy all .dll and .exe files from `build/` to your game directory
  (we recommend making a script file to do this)

## Compiling

### Compiling on Ubuntu

- **With Docker**:

    Make sure to install Docker and make, then run `make debug`.
    The binaries should appear in the `build/` directory.

- **Without Docker**:

    This scenario is not officially supported, but you can see how it's done by
    examining `docker/Dockerfile` and `docker/entrypoint.sh` for the external
    dependencies and `meson.build` for the local files, then tailoring your
    system to match the process.

### Compiling on Windows

- **Using WSL**:

    Install WSL (video guide: https://www.youtube.com/watch?v=5RTSlby-l9w)

    - Run Powershell as Administrator
    - Copy and paste the following command: `Enable-WindowsOptionalFeature -Online -FeatureName Microsoft-Windows-Subsystem-Linux`
    - Restart the computer
    - Go to Windows App Store
    - Install Ubuntu

    Run WSL and continue with the instructions from the `Compiling on Ubuntu` section.

### Supported compilers

Please be advised that any build systems that are not the one we use for
automating releases (= mingw-w64) come at user's own risk. They might crash or
even refuse to compile. Pull requests are welcome, but those other toolchains
will be always considered supplementary.

## Coding conventions

While the existing source code does not conform to the rules because it uses
the original Core Design's naming, new code should adhere to the following
guidelines:

- Variables are `lower_snake_case`
- Global variables are `g_PascalCase`
- Module variables are `m_PascalCase`
- Function names are `Module_PascalCase`
- Macros are `UPPER_SNAKE_CASE`
- Struct names are `UPPER_SNAKE_CASE`
- Struct members are `lower_snake_case`
- Enum names are `UPPER_SNAKE_CASE`
- Enum members are `UPPER_SNAKE_CASE`

Try to avoid global variables, if possible declare them as `static` in the
module you're using them. Changes to original game functionality most often
should be configurable.

Other things:

- We use clang-format to automatically format the code
- We do not omit `{` and `}`
- We use K&R brace style
- We condense `if` expressions into one, so:
    ```
    if (a && b) {
    }
    ```

    and not:

    ```
    if (a) {
        if (b) {
        }
    }
    ```

    If the expressions are extraordinarily complex, we refactor these into
    smaller conditions or functions.

### Code formatting

This project uses `clang-format` to take care of automatic code formatting, and
`include-what-you-use` to remove unused `#include`s. To ensure your code
conforms to the standard, please run `make lint` after each commit. If for some
reason you can't run it, don't worry, our CI pipeline will show what needs to
be changed in case of mistakes.

## Submitting changes

We commit via pull requests and avoid committing directly to `develop`, which
is a protected branch. Each pull request gets peer-reviewed and should have at
least one approval from the code developer team before merging. We never merge
until all discussions are marked as resolved and generally try to test things
before merging. When a remark on the code review is trivial and the PR author
has pushed a fix for it, it should be resolved by the pull request author.
Otherwise we don't mark the discussions as resolved and give a chance for the
reviewer to reply. Once all change requests are addressed, we should re-request
a review from the interested parties.

## Changelog

We keep a changelog in `CHANGELOG.md`. Anything other than an internal change
or refactor needs an entry there. Furthermore, new features and OG bugfixes
should be documented in README as well. If your change modifies gameflow
behavior, make sure to update `GAMEFLOW.md` as appropriate.

## Commit scope

Either you can make a lot of throwaway commits such as 'test' 'continue
testing' 'fix 1' 'fix 2' 'fix of the fix' and then squash your pull request as
a whole, or you can craft a nice history with proper commit messages and then
merge-rebase. The first case is mostly acceptable for isolated features, but in
general we prefer the latter approach. As a principle, refactors should made in
separate commits. Code review changes are best made incrementally and then
squashed prior to merging, for easing the review process.

## Commit messages

**The most important thing to remember:** bug fixes and feature implementations
should always include the phrase `Resolves #123`. If there's no ticket and the
pull request you're making contains player-facing changes, a ticket needs
to be created first – no exceptions.

Anything else is just for consistency and general neatness. Our commit messages
aim to respect the 50/72 rule and have the following form:

    module-prefix: description in an imperative mood (max 50 characters)

    Longer description of what happens that can span multiple lines. Each
    line should be maximally 72 characters long, with the exceptions of
    code/log dumps.

The prefix should describe the module that the pull request touches the most.
In general this is the name of the `.c` or `.h` file with the most changes.
Note that this includes the folder names which are separated with `/`. Avoid
underscores (`_`) in favor of dashes (`-`).

The description should be as concise as possible; any details should be given
in the commit message body. Use simple, to the point words like `add`, `fix`,
`remove`, `improve`.

Good:
```
ui: improve resolution changing

Added the ability for the player to switch resolutions directly from
the game ui.

Resolves #123.
```

Great:
```
log: fix varargs for Log_Message()

On Linux, the engine crashes when printing the log messages. This
happens because the current code re-uses the same va_list variable on
two calls to vprintf() and vfprintf(). Actually, this is not allowed.
For using the same information on multiple formatting functions, it is
needed to create a copy of the primary va_list to a second one, by using
va_copy(). After rewriting properly the Log_Message() function, the
segmentation fault is gone. Tested on both Linux and Windows builds.
```

- This has no ticket number, but it was an internal change improving support
  for a platform unsupported at that time, which made it acceptable.

Bad:
```
ui: implemented the ability to switch resolutions from the ui
```
- the subject doesn't use imperative mood
- the subject is too long
- missing ticket number

Bad:
```
dart: added dart emitters to the savegame (#779)

dart: added dart emitters to the savegame

Add function for checking legacy savegame save flags
Resolves #774.
```

- it duplicates the subject in the message body
- the subject doesn't use imperative mood

When merging via squash, it's OK to have GitHub append the pull request number,
but pay special attention to the body field which often gets filled with
garbage.

## Branching model

We have two branches: `develop` and `stable`. `develop` is where all changes
about to be published in the next release land. `stable` is the latest release.
We avoid creating merge commits between these two – they should always point to
the same HEAD when applicable. This means that any hotfixes that need to be
released ahead of unpublished work in `develop` are merged directly to
`stable`, and `develop` needs to be then rebased on top of the now-patched
`stable`.

## Releasing a new version

New version releases happen automatically whenever a new tag is pushed to the
`stable` branch with the help of GitHub actions. In general this is accompanied
with a special commit `docs: release X.Y.Z` that assigns unreleased changes to
a specific version. See git history for details.

## Glossary

- Tomb1Main: the previous name of this project
- T1M: short hand of Tomb1Main
- OG: original game, most often TombATI
- PS: PlayStation version of the game
- Vole: a rat that swims
- Pod: a mutant egg (including the big egg)
- Cabin: the room with the pistols from Natla's Mines
- Statue: centaur statues from the entrance of Tihocan's Tomb
- Bacon Lara: the doppelgänger Lara in the Atlantis level
- Torso/Adam: the big boss mutant from The Great Pyramid level
