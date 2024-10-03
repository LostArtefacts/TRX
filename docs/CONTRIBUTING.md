# Development guidelines

## Build workflow

Initial build:

- Compile the project (described in the next section)
- Copy all executable files from `build/` to your game directory
- Copy the contents of `data/ship/` to your game directory

Subsequent builds:

- Compile the project
- Copy all executable files from `build/` to your game directory
  (we recommend making a script file to do this)



## Compiling

### Compiling on Ubuntu

- **With Docker**:

    Make sure to install Docker and [just](https://github.com/casey/just).
    To see the list of all possible build targets, run `just -l`. To build the
    images, use the `just *-build-*` commands relevant to the game and platform
    you want to build for. The binaries should appear in the `build/`
    directory.

- **Without Docker**:

    This scenario is not officially supported, but you can see how it's done by
    examining the files in the `tools/*/docker/` directory for the external
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
even refuse to compile.



## Working with the project

### Top values

- Compatibility with the original game's look and feel
- Player choice whether to enable any impactful changes
- Maintainability
- Automation where possible
- Documentation (git history and GitHub issues are great for this purpose)

### Automatic code formatting

This project uses [pre-commit](https://pre-commit.com/) to make sure the code
is formatted the right way. This tool has additional external dependencies:
`clang-format` for automatic code formatting. To install pre-commit:

```
python3 -m pip install --user pre-commit
pre-commit install
```

To install required external dependencies on Ubuntu:

```
apt-get install -y clang-format-18
```

After this, each commit should trigger a hook to automatically format changes.
To manually initiate this process, run `just lint-format`. This excludes the
slower checks that could affect productivity – for the full process, run `just
lint`. If installing the above software isn't possible, the CI pipeline will
indicate necessary changes in case of mistakes.


### Coding convention

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


### Submitting changes

We commit via pull requests rather than directly to the protected `develop`
branch. Each pull request undergoes a peer review and requires at least one
approval from the development team before merging. We ensure that all
discussions are resolved and aim to test changes prior to merging. When a code
review comment is minor and the author has addressed it, they should mark it as
resolved. Otherwise, we leave discussions open to allow reviewers to respond.
After addressing all change requests, it's considerate to re-request a review
from the relevant parties.


### Changelog

We maintain a changelog for each project in the `CHANGELOG.md` files, recording
any changes except internal modifications or refactors. New features and
original bug fixes should also be documented in the `README.md`. If a change
affects gameflow behavior, be sure to update the `GAMEFLOW.md` accordingly.
Likewise, changes to the console commands should update `COMMANDS.md`.


### Commit scope

When merging, we use rebasing for a clean commit history. For that reason,
each significant change should have an isolated commit. It's okay to force-push
pull requests.


### Commit messages

**Bug fixes and feature implementations should include the phrase `Resolves
#123`.** For player-facing changes without an existing ticket, a ticket needs
to be created first.

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

> [!NOTE]
> This has no ticket number, but it was an internal change improving support
> for a platform unsupported at that time, which made it acceptable.

Bad:

```
ui: implemented the ability to switch resolutions from the ui
```

- the subject doesn't use imperative mood
- the subject is too long
- it's missing a ticket number

Bad:

```
dart: added dart emitters to the savegame (#779)

dart: added dart emitters to the savegame

Add function for checking legacy savegame save flags
Resolves #774.
```

- it duplicates the subject in the message body
- the subject doesn't use imperative mood

When using squash to merge, it is acceptable for GitHub to append the pull
request number, but it's important to carefully review the body field, as it
often includes unwanted content.


### Branching model

We have two branches: `develop` and `stable`. `develop` is where all changes
about to be published in the next release land. `stable` is the latest release.
We avoid creating merge commits between these two – they should always point to
the same HEAD when applicable. This means that any hotfixes that need to be
released ahead of unpublished work in `develop` are merged directly to
`stable`, and `develop` needs to be then rebased on top of the now-patched
`stable`.


### Tooling

Internal tools are typically coded in a reasonably recent version of Python,
while avoiding the use of bash, shell, and similar languages.


### Releasing a new version

New version releases happen automatically whenever a new tag is pushed to the
`stable` branch with the help of GitHub actions. In general this is accompanied
with a special commit `docs: release X.Y.Z` that also adjusts the changelog.
See git history for details.



## Glossary

- OG: original game (for TR1 this is most often TombATI)
- PS: the PlayStation version of the game
- UK Box: a variant of Tomb Raider II released on physical discs in the UK
- Multipatch: a variant of Tomb Raider II released on Steam
- Vole: a rat that swims
- Pod: a mutant egg (including the big egg)
- Cabin: the room with the pistols from Natla's Mines
- Statue: centaur statues from the entrance of Tihocan's Tomb
- Bacon Lara: the doppelgänger Lara in the Atlantis level
- Torso/Adam: the big boss mutant from The Great Pyramid level
- Tomb1Main: the previous name of this TR1X project
- T1M: short hand of Tomb1Main
