---
title: Console
order: 24
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/console.lua. Edit it there.
-->

## <a id="console" name="console"></a>Console module

Module for interacting with the developer console.

[`trx.console.log`](#console.log) writes to the console overlay in-game, where [`trx.log`](LOG.md#log) writes only to the terminal and the log file.

### Enums

- <a id="console.Result" name="console.Result"></a>[lua]`trx.console.Result`

    How a console command went. What a command's [`trx.console.register.spec.run`](#console.register.spec.run) gives back.

    - `trx.console.Result.OK` = `0`  
        It worked.
    - `trx.console.Result.FAILURE` = `1`  
        It ran and could not do what was asked.
    - `trx.console.Result.UNAVAILABLE` = `2`  
        It cannot run here - no level is loaded, or the game is in a menu.
    - `trx.console.Result.BAD_INVOCATION` = `3`  
        The player typed it wrong.

### Structures

- <a id="console.Command" name="console.Command"></a>[lua]`trx.console.Command`

    A registered console command, as the help command reads one.

    Properties:
    - <a id="console.Command.aliases" name="console.Command.aliases"></a>**`aliases`**: a list of string, optional. The other words that reach it, where it answers to more than one.
    - <a id="console.Command.help" name="console.Command.help"></a>**`help`**: string, optional. What the console shows for `--help`, where the command carries any.
    - <a id="console.Command.name" name="console.Command.name"></a>**`name`**: string. The word the player types.

### Functions

- <a id="console.log" name="console.log"></a>[lua]`trx.console.log(message)`  
  Logs a line to the developer console. Calling the group itself logs at `INFO`. Takes any value: a table is pretty-printed, anything else coerced to a string.

  Parameters:
  - <a id="console.log.message" name="console.log.message"></a>**`message`** (any). Any value; a table is pretty-printed.

  Example:
  ```lua
  trx.console.log({ hp = 1000, pos = { x = 1 } })
  ```

- <a id="console.log.generic" name="console.log.generic"></a>[lua]`trx.console.log.generic(level, message)`  
  Logs at a level chosen at runtime.

  Parameters:
  - <a id="console.log.generic.level" name="console.log.generic.level"></a>**`level`** ([trx.log.LogLevel](LOG.md#log.LogLevel)).
  - <a id="console.log.generic.message" name="console.log.generic.message"></a>**`message`** (any). Any value; a table is pretty-printed.

- <a id="console.log.info" name="console.log.info"></a>[lua]`trx.console.log.info(message)`  
  Logs an informational message.

  Parameters:
  - <a id="console.log.info.message" name="console.log.info.message"></a>**`message`** (any). Any value; a table is pretty-printed.

- <a id="console.log.warn" name="console.log.warn"></a>[lua]`trx.console.log.warn(message)`  
  Logs a warning.

  Parameters:
  - <a id="console.log.warn.message" name="console.log.warn.message"></a>**`message`** (any). Any value; a table is pretty-printed.

- <a id="console.log.warning" name="console.log.warning"></a>[lua]`trx.console.log.warning(message)`  
  Logs a warning. An alias of [`warn`](#console.log.warn).

  Parameters:
  - <a id="console.log.warning.message" name="console.log.warning.message"></a>**`message`** (any). Any value; a table is pretty-printed.

- <a id="console.log.error" name="console.log.error"></a>[lua]`trx.console.log.error(message)`  
  Logs an error.

  Parameters:
  - <a id="console.log.error.message" name="console.log.error.message"></a>**`message`** (any). Any value; a table is pretty-printed.

- <a id="console.log.debug" name="console.log.debug"></a>[lua]`trx.console.log.debug(message)`  
  Logs a debug message.

  Parameters:
  - <a id="console.log.debug.message" name="console.log.debug.message"></a>**`message`** (any). Any value; a table is pretty-printed.

- <a id="console.eval" name="console.eval"></a>[lua]`trx.console.eval(command, [opts])`  
  Runs a developer console command. Raises if the command fails.

  Output appears only in the terminal and the log file by default. Pass `{ verbose = true }` to show it in the console, or `{ capture = true }` to return the logged text.

  Parameters:
  - <a id="console.eval.command" name="console.eval.command"></a>**`command`** (string). Command to run, as the player would type it.
  - <a id="console.eval.opts" name="console.eval.opts"></a>**`opts`** (table, optional). How to run it.

    Keys:
    - <a id="console.eval.opts.verbose" name="console.eval.opts.verbose"></a>**`verbose`** (boolean, optional). Show the command's output.
    - <a id="console.eval.opts.capture" name="console.eval.opts.capture"></a>**`capture`** (boolean, optional). Return the command's output.

  Returns: string or `nil`. What the command logged, one line per message. `nil` unless [`opts.capture`](#console.eval.opts.capture) is true.

  Example:
  ```lua
  trx.console.eval("play 1", { verbose = true })
  ```

- <a id="console.copy" name="console.copy"></a>[lua]`trx.console.copy(text)`  
  Puts text in the system clipboard. Raises if the platform refuses it.

  Parameters:
  - <a id="console.copy.text" name="console.copy.text"></a>**`text`** (string). What to put in the clipboard.

  Example:
  ```lua
  trx.console.copy(trx.game.TRX_VERSION)
  ```

- <a id="console.complete" name="console.complete"></a>[lua]`trx.console.complete(line, caret)`  
  Completes a console line with the same suggestions as the prompt. Use it when a Lua command wraps another console command.

  Parameters:
  - <a id="console.complete.line" name="console.complete.line"></a>**`line`** (string). The line so far, without the key that opens the console.
  - <a id="console.complete.caret" name="console.complete.caret"></a>**`caret`** (integer). Where the caret sits, as a byte offset.

  Returns:
  - a list of string. The best match comes first.
  - integer. Where the run they replace starts.
  - integer. Where that run ends.

- <a id="console.register" name="console.register"></a>[lua]`trx.console.register(spec)`  
  Registers a console command written in Lua.

  Every command has a [`trx.argparse`](ARGPARSE.md#argparse) parser. [`spec.args`](#console.register.spec.args) is an optional function that shapes it - it receives the parser and declares the arguments the command takes. A command that omits [`spec.args`](#console.register.spec.args) takes none, and reports so when handed one. The console completes the arguments from the parser, and answers `-h`/`--help` from it.

  [`spec.run`](#console.register.spec.run) receives the parsed values, a table keyed by argument name. What it gives back is a [`trx.console.Result`](#console.Result), and returning nothing means `OK`. It may return a message after that, which is logged to the console - as an error, for any result but `OK`. A line the parser rejects is reported with what it expected, without reaching [`spec.run`](#console.register.spec.run).

  The console completes arguments from the parser by default. [`spec.complete`](#console.register.spec.complete) replaces that behavior, so a command that wraps [`trx.console.eval`](#console.eval) can answer from [`trx.console.complete`](#console.complete).

  A command lives for the whole run, so it can only be registered from a global script. A level script raises if it calls this: it runs again every time its level is loaded.

  Parameters:
  - <a id="console.register.spec" name="console.register.spec"></a>**`spec`** (table). The command.

    Keys:
    - <a id="console.register.spec.name" name="console.register.spec.name"></a>**`name`** (string). The word the player types.
    - <a id="console.register.spec.help" name="console.register.spec.help"></a>**`help`** (string, optional). A game string key for the help text.
    - <a id="console.register.spec.args" name="console.register.spec.args"></a>**`args`** (function, optional). Shapes the parser.
    - <a id="console.register.spec.run" name="console.register.spec.run"></a>**`run`** (function). Called with the parsed arguments.
    - <a id="console.register.spec.complete" name="console.register.spec.complete"></a>**`complete`** (function, optional). Completes arguments instead of the parser. Called with the text past the command word and the caret byte offset in it, and gives back what [`trx.argparse.Parser:complete`](ARGPARSE.md#argparse.Parser.complete) does.
    - <a id="console.register.spec.aliases" name="console.register.spec.aliases"></a>**`aliases`** (a list of string, optional). Other words that reach the same command. They dispatch but stay out of the command listing, and the help for the command shows them.

  Example:
  ```lua
  trx.console.register({
    name = "greet",
    aliases = { "hello", "hi" },
    args = function(parser)
      parser:positional("who", { help = "who to greet" })
    end,
    run = function(args)
      trx.console.log("hello " .. args.who)
    end,
  })
  ```

- <a id="console.clear" name="console.clear"></a>[lua]`trx.console.clear()`  
  Clears the console.

- <a id="console.commands" name="console.commands"></a>[lua]`trx.console.commands()`  
  Every registered console command, in registration order. The help command is built on this.

  Returns: a list of [trx.console.Command](#console.Command).

- <a id="console.command" name="console.command"></a>[lua]`trx.console.command(name)`  
  The command a name reaches, by its own name or an alias, matched as the console matches when it dispatches.

  Parameters:
  - <a id="console.command.name" name="console.command.name"></a>**`name`** (string). The word or alias to look up.

  Returns: [trx.console.Command](#console.Command) or `nil`. `nil` when nothing answers to the name.
