local raw = trxc.console
local h = require("trx.internal.helpers")

require("trx.locale")

-- trx.console.log.generic takes a trx.log.LogLevel, and the log module is what
-- declares it.
local LogLevel = trx.log.LogLevel

---@class trx
---@field console trx.console

---Module for interacting with the developer console.
---
---`trx.console.log` writes to the console overlay in-game, where `trx.log`
---writes only to the terminal and the log file.
---@trx.module 25
---@class (exact) trx.console
---@field is_open boolean Whether the console is open.
---
---  Assigning opens or closes it, as the key the player bound to it does.
local M = h.module("console")

trx.locale.declare({
  ["console/argparse/aliases"] = "Aliases: %s",
})

-- Renders any value for the console. Nested strings are quoted so a table reads
-- back the way it was written; a table prints across lines, indented.
local function render(value, indent, seen)
  local kind = type(value)
  if kind ~= "table" then
    return kind == "string" and ("%q"):format(value) or tostring(value)
  end
  if seen[value] then
    return "<cycle>"
  end
  seen[value] = true

  local inner = indent .. "  "
  local parts, len = {}, 0
  for i, item in ipairs(value) do
    parts[#parts + 1] = inner .. render(item, inner, seen)
    len = i
  end

  local keys = {}
  for key in pairs(value) do
    local in_seq = type(key) == "number"
      and key % 1 == 0
      and key >= 1
      and key <= len
    if not in_seq then
      keys[#keys + 1] = key
    end
  end
  table.sort(keys, function(a, b)
    return tostring(a) < tostring(b)
  end)
  for _, key in ipairs(keys) do
    local label = (type(key) == "string" and key:match("^[%a_][%w_]*$"))
        and key
      or "[" .. render(key, inner, seen) .. "]"
    parts[#parts + 1] = inner
      .. label
      .. " = "
      .. render(value[key], inner, seen)
  end

  seen[value] = nil
  if #parts == 0 then
    return "{}"
  end
  return "{\n" .. table.concat(parts, ",\n") .. "\n" .. indent .. "}"
end

-- A string logs as itself; anything else is coerced, and a table is pretty-
-- printed.
local function to_display(value)
  if type(value) == "string" then
    return value
  end
  return render(value, "", {})
end

local function at(level)
  return function(value)
    raw.log(level, to_display(value))
  end
end

---How a console command went. What a command's `trx.console.register.spec.run`
---gives back.
---@enum trx.console.Result
local Result = {
  OK = "It worked.",
  FAILURE = "It ran and could not do what was asked.",
  UNAVAILABLE = "It cannot run here - no level is loaded, or the game is in a menu.",
  BAD_INVOCATION = "The player typed it wrong.",
}
M.Result = h.enum("console.Result", "COMMAND_RESULT", Result)

local is_result = {}
for _, value in pairs(M.Result) do
  is_result[value] = true
end

-- The "Aliases: a, b" line a command's help ends with, or empty when it has
-- none. aliases is the comma-joined display string.
local function aliases_line(aliases)
  if aliases == nil then
    return ""
  end
  return "\n" .. trx.locale.format("console/argparse/aliases", aliases)
end

-- What a command prints for `--help`: its description, the synopsis its parser
-- gives, and its aliases.
local function help_text(help_key, parser, aliases)
  local text = parser:usage()
  if help_key ~= nil then
    text = trx.locale.get(help_key) .. "\n\n" .. text
  end
  return text .. aliases_line(aliases)
end

-- Every command registered from Lua, by name, holding what its help is composed
-- from. The help command reads this to answer for a command the way that command
-- answers `--help`, so the two cannot drift.
local commands = {}

-- The logging functions take any value: a string logs as itself, and anything
-- else is coerced, with a table pretty-printed across lines.

---Logs a line to the developer console. Calling the group itself logs at
---`INFO`. Takes any value: a table is pretty-printed, anything else coerced to
---a string.
---
---```lua
---trx.console.log({ hp = 1000, pos = { x = 1 } })
---```
---@class (exact) trx.console.log
---@param message any Any value; a table is pretty-printed.
---@overload fun(message: any)
M.log = h.namespace("console.log", at(LogLevel.INFO))

---A registered console command, as the help command reads one.
---@trx.record
---@class trx.console.Command
---@field name string The word the player types.
---@field aliases? string[] The other words that reach it, where it answers to more than one.
---@field help? string What the console shows for `--help`, where the command carries any.

---Logs at a level chosen at runtime.
---@param level trx.log.LogLevel
---@param message any Any value; a table is pretty-printed.
function M.log.generic(level, message)
  raw.log(level, to_display(message))
end

---Logs an informational message.
---@param message any Any value; a table is pretty-printed.
---@type fun(message: any)
M.log.info = at(LogLevel.INFO)

---Logs a warning.
---@param message any Any value; a table is pretty-printed.
---@type fun(message: any)
M.log.warn = at(LogLevel.WARNING)

---Logs a warning. An alias of `trx.console.log.warn`.
---@param message any Any value; a table is pretty-printed.
---@type fun(message: any)
M.log.warning = at(LogLevel.WARNING)

---Logs an error.
---@param message any Any value; a table is pretty-printed.
---@type fun(message: any)
M.log.error = at(LogLevel.ERROR)

---Logs a debug message.
---@param message any Any value; a table is pretty-printed.
---@type fun(message: any)
M.log.debug = at(LogLevel.DEBUG)

---@class (exact) trx.console.eval.opts
---@field verbose? boolean Show the command's output.
---@field capture? boolean Return the command's output.

---Runs a developer console command. Raises if the command fails.
---
---Output appears only in the terminal and the log file by default. Pass `{
---verbose = true }` to show it in the console, or `{ capture = true }` to
---return the logged text.
---
---```lua
---trx.console.eval("play 1", { verbose = true })
---```
---@param command string Command to run, as the player would type it.
---@param opts? trx.console.eval.opts How to run it.
---@return string? # What the command logged, one line per message. `nil` unless `trx.console.eval.opts.capture` is true.
---@type fun(command: string, opts?: trx.console.eval.opts): string?
M.eval = raw.eval

---Puts text in the system clipboard. Raises if the platform refuses it.
---
---Assigning `trx.ui.clipboard` does the same thing.
---
---```lua
---trx.console.copy(trx.game.TRX_VERSION)
---```
---@param text string What to put in the clipboard.
---@type fun(text: string)
M.copy = raw.copy

---Completes a console line with the same suggestions as the prompt. Use it
---when a Lua command wraps another console command.
---@param line string The line so far, without the key that opens the console.
---@param caret integer Where the caret sits, as a byte offset.
---@return string[] # The best match comes first.
---@return integer # Where the run they replace starts.
---@return integer # Where that run ends.
---@type fun(line: string, caret: integer): string[], integer, integer
M.complete = raw.complete

h.properties(M, "console", {
  is_open = { get = raw.is_open, set = raw.set_open },
})

---The lines the player has entered, oldest first.
---
---The list is a copy, so changing it changes nothing. Use
---`trx.console.remember` to add to it.
---@return string[] # The lines entered so far.
---@type fun(): string[]
M.history = raw.history

---Adds a line to what the player has entered.
---
---The console does this for every line the player runs, so a script needs it
---only where it takes lines of its own.
---@param line string The line to remember.
---@type fun(line: string)
M.remember = raw.remember

---@class (exact) trx.console.register.spec
---@field name string The word the player types.
---@field help? string A game string key for the help text.
---@field args? function Shapes the parser.
---@field run function Called with the parsed arguments.
---@field complete? function Completes arguments instead of the parser. Called with the text past the command word and the caret byte offset in it, and gives back what `trx.argparse.Parser:complete` does.
---@field aliases? string[] Other words that reach the same command. They dispatch but stay out of the command listing, and the help for the command shows them.

---Registers a console command written in Lua.
---
---Every command has a `trx.argparse` parser. `trx.console.register.spec.args`
---is an optional function that shapes it - it receives the parser and declares
---the arguments the command takes. A command that omits
---`trx.console.register.spec.args` takes none, and reports so when handed one.
---The console completes the arguments from the parser, and answers
---`-h`/`--help` from it.
---
---`trx.console.register.spec.run` receives the parsed values, a table keyed by
---argument name. What it gives back is a `trx.console.Result`, and returning
---nothing means `OK`. It may return a message after that, which is logged to
---the console - as an error, for any result but `OK`. A line the parser
---rejects is reported with what it expected, without reaching
---`trx.console.register.spec.run`.
---
---The console completes arguments from the parser by default.
---`trx.console.register.spec.complete` replaces that behavior, so a command
---that wraps `trx.console.eval` can answer from `trx.console.complete`.
---
---A command lives for the whole run, so it can only be registered from a
---global script. A level script raises if it calls this: it runs again every
---time its level is loaded.
---
---```lua
---trx.console.register({
---  name = "greet",
---  aliases = { "hello", "hi" },
---  args = function(parser)
---    parser:positional("who", { help = "who to greet" })
---  end,
---  run = function(args)
---    trx.console.log("hello " .. args.who)
---  end,
---})
---```
---@param spec trx.console.register.spec The command.
function M.register(spec)
  assert(type(spec) == "table", "trx.console.register expects a table")
  assert(
    type(spec.name) == "string",
    "trx.console.register: name must be a string"
  )
  assert(
    spec.help == nil or type(spec.help) == "string",
    "trx.console.register: help must be a string"
  )
  assert(
    type(spec.run) == "function",
    "trx.console.register: run must be a function"
  )
  assert(
    spec.aliases == nil or type(spec.aliases) == "table",
    "trx.console.register: aliases must be a table"
  )
  assert(
    spec.args == nil or type(spec.args) == "function",
    "trx.console.register: args must be a function"
  )
  assert(
    spec.complete == nil or type(spec.complete) == "function",
    "trx.console.register: complete must be a function"
  )

  local parser = trx.argparse.new({ prog = spec.name })
  if spec.args ~= nil then
    spec.args(parser)
  end

  local aliases = nil
  if spec.aliases ~= nil and #spec.aliases > 0 then
    aliases = table.concat(spec.aliases, ", ")
  end
  commands[spec.name] =
    { help = spec.help, parser = parser, aliases = aliases }

  raw.register(spec.name, spec.help, function(args)
    local parsed, err = parser:parse((args or ""):match("^%s*(.-)%s*$"))
    if parsed == nil then
      -- A parse error names what it expected, so it stands in for the console's
      -- generic one; FAILURE keeps that generic line from also being logged.
      trx.console.log.error(parser:format_error(err))
      return M.Result.FAILURE
    end
    if parsed.help then
      trx.console.log(help_text(spec.help, parser, aliases))
      return M.Result.OK
    end

    local result, message = spec.run(parsed)
    -- Only returning nothing means OK; `or` would take a `false` for it too.
    if result == nil then
      result = M.Result.OK
    end
    assert(
      is_result[result],
      spec.name .. ": run must give back a trx.console.Result"
    )
    if message ~= nil then
      if result == M.Result.OK then
        trx.console.log.info(message)
      else
        trx.console.log.error(message)
      end
    end
    return result
  end, spec.aliases, spec.complete or function(text, caret)
    return parser:complete(text or "", caret)
  end)
end

---Clears the console.
---@type fun()
M.clear = raw.clear

-- The aliases string the registry holds is comma-joined for display; a script
-- reads them as a list. Absent when the command has none.
local function split_aliases(joined)
  if joined == nil then
    return nil
  end
  local out = {}
  for alias in joined:gmatch("[^,%s]+") do
    out[#out + 1] = alias
  end
  return out
end

-- The help a command shows, or nil when it carries none. A command written in
-- Lua composes it from its parser, the same way it answers `--help`. One written
-- in C has no parser, so it falls back to the description and aliases the
-- registry holds. A command with no help id is undocumented either way, even
-- though its parser could still answer `--help` with a bare synopsis.
local function command_help(cmd)
  if cmd.help == nil then
    return nil
  end
  local record = commands[cmd.name]
  if record ~= nil then
    return help_text(record.help, record.parser, record.aliases)
  end
  return trx.locale.get(cmd.help) .. aliases_line(cmd.aliases)
end

-- A command as a script reads it: the word, its aliases as a list, and the text
-- the console shows for `--help`.
local function descriptor(cmd)
  return {
    name = cmd.name,
    aliases = split_aliases(cmd.aliases),
    help = command_help(cmd),
  }
end

---Every registered console command, in registration order. The help command is
---built on this.
---@return trx.console.Command[]
function M.commands()
  local out = {}
  for _, cmd in ipairs(raw.commands()) do
    out[#out + 1] = descriptor(cmd)
  end
  return out
end

---The command a name reaches, by its own name or an alias, matched as the
---console matches when it dispatches.
---@param name string The word or alias to look up.
---@return trx.console.Command? # `nil` when nothing answers to the name.
function M.command(name)
  local cmd = raw.command(name)
  if cmd == nil then
    return nil
  end
  return descriptor(cmd)
end

-- p is a global shorthand for trx.console.log, for quick debugging from the
-- console. The one global besides trx, and so documented by hand in MISC.md
-- rather than through the registry.
_G.p = trx.console.log
