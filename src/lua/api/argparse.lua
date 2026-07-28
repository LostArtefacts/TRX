local api = trx.api

require("trx.locale")

api.module("argparse", {
  order = 23,
  description = [[
A small, declarative argument parser for console commands, in the shape of
Python's argparse.

A parser both reads a command's arguments and offers completions for them,
from one declaration. Every command written with `trx.console.register` has
one; a command shapes it through the `args` function it hands over, and `run`
then receives a table of parsed values. A command that shapes nothing takes no
arguments, and is told so when given one.

Every parser answers `-h` and `--help` on its own, printing what it accepts.

How a positional reads a token is its `matcher`, one of:

- `type` - coerce to `"integer"`, `"number"`, `"string"` or `"boolean"`.
- `choices` - the allowed set: a list of values or a `function(parsed)`
  returning one. The token must match one; the set is shown in errors and
  completes.
- `match` - a `function(token, parsed)` returning `value, ok`, for a shape of its own.

These do not combine on one positional; a value that is a number *or* a name is
two matchers, declared with `any_of`. Separately, `suggest` offers completions
without restricting or being shown in errors - for a free value with a long
list behind it, like a setting name.

A parser has these methods, each returning the parser so calls chain:

- `positional(name, opts)` - a positional with one matcher (`opts.type`,
  `opts.choices` or `opts.match`). `opts.optional` lets it be left out;
  `opts.greedy` reads the rest of the line as one token, so a value with spaces
  in it still arrives whole; `opts.suggest` completes it; `opts.help` describes
  it.
- `any_of(name, alternatives, opts)` - a positional whose value is the first of
  several matchers to take the token. Each alternative is a matcher table,
  `{ type = ... }` or `{ choices = ... }`. Same `opts` as `positional`.
- `rest(name, opts)` - the rest of the line from here on, verbatim as one
  string, or nil when an optional one is absent. Always the last argument;
  `opts.suggest` completes it.
- `flag(name, opts)` - a boolean that may sit anywhere. `opts.short` and
  `opts.long` are the spellings, e.g. `"-f"` and `"--force"`; `opts.help`
  describes it.
- `parse(args)` - reads the argument string, returning a table of values, or
  `nil` and a structured error the console layer turns into localized text. A
  value carried by a `{ key, value }` choice comes back as its `value`;
  `-h`/`--help` comes back as `{ help = true }`.
- `complete(text, caret)` - the candidate completions for the token the caret
  sits in, and the byte offsets `start, end` of the run they replace (reaching
  to the end of the line for a greedy argument).
- `usage()` - a short description of what the command accepts.

A choice is either a bare string, where the key and value are the same, or a
`{ key, value }` pair, where `key` is matched and shown and `value` is what
`parse` gives back. Matching is forgiving, through `trx.strings.fuzzy_match`.]],
})

trx.locale.declare({
  ["console/argparse/expected"] = "Expected %s.",
  ["console/argparse/hint_boolean"] = "on or off",
  ["console/argparse/hint_integer"] = "a whole number",
  ["console/argparse/hint_number"] = "a number",
  ["console/argparse/hint_or"] = "%s, or %s",
  ["console/argparse/hint_values"] = "one of: %s",
  ["console/argparse/invalid"] = "Invalid %s: %s.",
  ["console/argparse/missing"] = "Missing %s.",
  ["console/argparse/unexpected"] = "Unexpected argument: %s.",
  ["console/argparse/usage"] = "Usage: %s",
})

-- A choice is a bare string (key == value) or a { key, value } pair.
local function normalize_choices(raw)
  local out = {}
  for _, choice in ipairs(raw) do
    if type(choice) == "table" then
      out[#out + 1] = { key = choice.key, value = choice.value }
    else
      out[#out + 1] = { key = choice, value = choice }
    end
  end
  return out
end

-- The choices a source (a list or a function) offers, resolved against what has
-- parsed so far.
local function choices_from(source, parsed)
  if source == nil then
    return nil
  end
  if type(source) == "function" then
    source = source(parsed)
  end
  return normalize_choices(source or {})
end

-- choices_from, but a source function that raises yields no choices rather than
-- propagating out of parse or complete. match_one, hint_of and candidates_for
-- all go through this, so a bad runtime choices list is a clean rejection.
local function safe_choices(source, parsed)
  local ok, choices = pcall(choices_from, source, parsed)
  if not ok then
    return nil
  end
  return choices
end

local function coerce_type(kind, token)
  if kind == "integer" then
    if token:match("^%-?%d+$") then
      return tonumber(token), true
    end
    return nil, false
  elseif kind == "number" then
    -- Plain decimal, to match integer's grammar; tonumber alone would also take
    -- hex and scientific forms a player is not expecting to type.
    if
      token:match("^%-?%d*%.?%d+$") == nil
      and token:match("^%-?%d+%.%d*$") == nil
    then
      return nil, false
    end
    return tonumber(token), true
  elseif kind == "boolean" then
    local value = trx.strings.parse_bool(token)
    return value, value ~= nil
  end
  return token, true
end

-- Turns one token into a value through a single matcher. Returns value, ok.
local function match_one(matcher, token, parsed)
  if matcher.match ~= nil then
    return matcher.match(token, parsed)
  end
  if matcher.type ~= nil then
    return coerce_type(matcher.type, token)
  end
  if matcher.choices ~= nil then
    local choices = safe_choices(matcher.choices, parsed)
    if choices == nil then
      return nil, false
    end
    local sources = {}
    for _, choice in ipairs(choices) do
      sources[#sources + 1] = { key = choice.key, value = choice.value }
    end
    local matches = trx.strings.fuzzy_match(token, sources)
    if #matches > 0 then
      return matches[1].value, true
    end
    return nil, false
  end
  -- A matcher with no kind takes any token as a plain string.
  return token, true
end

-- The value for an argument: the first of its matchers to take the token.
local function resolve(arg, token, parsed)
  for _, matcher in ipairs(arg.matchers) do
    local value, ok = match_one(matcher, token, parsed)
    if ok then
      return value, true
    end
  end
  return nil, false
end

-- What an argument accepts, as data the caller turns into words: a coercing type
-- and the choice keys across its matchers. `suggest` is left out - it is for
-- completion, not for telling the player what fits. Returns nil for a free
-- value.
local function hint_of(arg, parsed)
  local htype, values = nil, nil
  for _, matcher in ipairs(arg.matchers) do
    if matcher.type == "boolean" then
      return { type = "boolean" }
    end
    if matcher.type ~= nil and matcher.type ~= "string" and htype == nil then
      htype = matcher.type
    end
    if matcher.choices ~= nil then
      local choices = safe_choices(matcher.choices, parsed)
      if choices ~= nil and #choices > 0 then
        values = values or {}
        for _, choice in ipairs(choices) do
          values[#values + 1] = choice.key
        end
      end
    end
  end
  if htype == nil and values == nil then
    return nil
  end
  return { type = htype, values = values }
end

-- Turns a hint into localized words: "a whole number, or one of: snow, rain".
-- Only the console layer prints these, so this reaches for trx.locale, while
-- parse stays pure and returns the hint as data.
local function format_hint(hint)
  if hint == nil then
    return nil
  end
  if hint.type == "boolean" then
    return trx.locale.get("console/argparse/hint_boolean")
  end

  local parts = {}
  if hint.type == "integer" then
    parts[#parts + 1] = trx.locale.get("console/argparse/hint_integer")
  elseif hint.type == "number" then
    parts[#parts + 1] = trx.locale.get("console/argparse/hint_number")
  end
  if hint.values ~= nil then
    parts[#parts + 1] = trx.locale.format(
      "console/argparse/hint_values",
      table.concat(hint.values, ", ")
    )
  end

  if #parts == 0 then
    return nil
  end
  local out = parts[1]
  for i = 2, #parts do
    out = trx.locale.format("console/argparse/hint_or", out, parts[i])
  end
  return out
end

local Parser = {}
Parser.__index = Parser

function Parser.new(spec)
  spec = spec or {}
  local self = setmetatable({
    prog = spec.prog,
    description = spec.description,
    positionals = {},
    -- Every parser answers -h/--help on its own. The flag is placed directly
    -- rather than through :flag, which reserves the name against reuse.
    flags = {
      {
        name = "help",
        short = "-h",
        long = "--help",
        help = "show this help",
      },
    },
  }, Parser)
  return self
end

-- Builds one matcher from a value table, refusing to fold several ways of reading
-- a token into one: a number *or* a name is `any_of`, not a positional carrying
-- both a type and choices.
local function make_matcher(opts)
  local kinds = 0
  for _, key in ipairs({ "type", "choices", "match" }) do
    if opts[key] ~= nil then
      kinds = kinds + 1
    end
  end
  assert(
    kinds <= 1,
    "argparse: an argument reads a token one way; use any_of for several"
  )
  -- metavar and help let an alternative name and describe itself, so help can
  -- list `status`, `stop` and `id` on lines of their own.
  return {
    type = opts.type,
    choices = opts.choices,
    match = opts.match,
    metavar = opts.metavar,
    help = opts.help,
  }
end

function Parser:positional(name, opts)
  opts = opts or {}
  self.positionals[#self.positionals + 1] = {
    name = name,
    optional = opts.optional == true,
    -- A greedy argument reads the rest of the line as one token, so a value with
    -- spaces in it (a level title) still arrives whole.
    greedy = opts.greedy == true,
    metavar = opts.metavar or name,
    help = opts.help,
    suggest = opts.suggest,
    matchers = { make_matcher(opts) },
  }
  return self
end

function Parser:any_of(name, alternatives, opts)
  opts = opts or {}
  local matchers = {}
  for _, alt in ipairs(alternatives) do
    matchers[#matchers + 1] = make_matcher(alt)
  end
  self.positionals[#self.positionals + 1] = {
    name = name,
    optional = opts.optional == true,
    greedy = opts.greedy == true,
    metavar = opts.metavar or name,
    help = opts.help,
    suggest = opts.suggest,
    matchers = matchers,
  }
  return self
end

-- The rest of the line, from here on, verbatim as one string. Required unless
-- `opts.optional`; when it is optional and nothing is left, it comes back as
-- nil. Always the last argument. A plain-string matcher takes the line whole.
function Parser:rest(name, opts)
  opts = opts or {}
  self.positionals[#self.positionals + 1] = {
    name = name,
    optional = opts.optional == true,
    greedy = true,
    metavar = opts.metavar or name,
    help = opts.help,
    suggest = opts.suggest,
    matchers = { {} },
  }
  return self
end

function Parser:flag(name, opts)
  opts = opts or {}
  assert(name ~= "help", "argparse: 'help' is a reserved flag name")
  self.flags[#self.flags + 1] = {
    name = name,
    short = opts.short,
    long = opts.long,
    help = opts.help,
  }
  return self
end

-- Splits on whitespace, each token keeping the byte offset it starts at, so a
-- `rest` positional can hand back the original input from that point, verbatim,
-- and completion can point at where a token begins.
local function tokenize_offsets(s)
  local out = {}
  local init = 1
  while true do
    local a, b = s:find("%S+", init)
    if a == nil then
      break
    end
    out[#out + 1] = { text = s:sub(a, b), start = a }
    init = b + 1
  end
  return out
end

function Parser:flag_for(token)
  for _, flag in ipairs(self.flags) do
    if token == flag.short or token == flag.long then
      return flag
    end
  end
  return nil
end

local function missing(arg, values)
  return {
    kind = "missing",
    metavar = arg.metavar,
    hint = hint_of(arg, values),
  }
end

function Parser:parse(args)
  args = args or ""
  local values = {}
  for _, flag in ipairs(self.flags) do
    values[flag.name] = false
  end

  local toks = tokenize_offsets(args)
  local pos_idx = 1
  local i = 1

  while i <= #toks do
    local tok = toks[i]
    local flag = self:flag_for(tok.text)
    if flag ~= nil then
      -- Help answers the moment it is seen, ahead of any later token that would
      -- otherwise fail first. A greedy positional swallows a -h that follows it,
      -- so it never reaches here as a flag.
      if flag.name == "help" then
        return { help = true }
      end
      values[flag.name] = true
      i = i + 1
    else
      local arg = self.positionals[pos_idx]
      if arg == nil then
        return nil, { kind = "unexpected", token = tok.text }
      end
      -- A greedy argument reads the rest of the line as one token; the plain
      -- token otherwise.
      local token = arg.greedy and args:sub(tok.start) or tok.text
      local value, ok = resolve(arg, token, values)
      if not ok then
        return nil,
          {
            kind = "invalid",
            metavar = arg.metavar,
            token = token,
            hint = hint_of(arg, values),
          }
      end
      values[arg.name] = value
      pos_idx = pos_idx + 1
      i = arg.greedy and #toks + 1 or i + 1
    end
  end

  -- Whatever went unfilled: an optional argument stays nil, a required one is
  -- missing.
  for k = pos_idx, #self.positionals do
    local arg = self.positionals[k]
    if not arg.optional then
      return nil, missing(arg, values)
    end
  end

  return values
end

-- Turns a parse error into a localized, capitalized line naming what was wrong
-- and what was expected.
function Parser:format_error(err)
  local msg
  if err.kind == "missing" then
    msg = trx.locale.format("console/argparse/missing", err.metavar)
  elseif err.kind == "invalid" then
    msg = trx.locale.format("console/argparse/invalid", err.metavar, err.token)
  else
    msg = trx.locale.format("console/argparse/unexpected", err.token)
  end
  local hint = format_hint(err.hint)
  if hint ~= nil then
    msg = msg .. " " .. trx.locale.format("console/argparse/expected", hint)
  end
  return msg
end

-- The keys an argument offers for `active`, best first. An empty `active` offers
-- them all. Both the choices its matchers restrict to and its `suggest` list
-- contribute; a boolean offers on/off without being told to.
local function candidates_for(arg, parsed, active)
  local out = {}
  local seen = {}
  local function emit(key)
    if not seen[key] then
      seen[key] = true
      out[#out + 1] = key
    end
  end
  local function add(choices)
    if choices == nil then
      return
    end
    if active == "" then
      for _, choice in ipairs(choices) do
        emit(choice.key)
      end
      return
    end
    local sources = {}
    for _, choice in ipairs(choices) do
      sources[#sources + 1] = { key = choice.key, value = choice.key }
    end
    for _, match in ipairs(trx.strings.fuzzy_match(active, sources)) do
      emit(match.value)
    end
  end

  for _, matcher in ipairs(arg.matchers) do
    local choices = safe_choices(matcher.choices, parsed)
    if choices == nil and matcher.type == "boolean" then
      choices = normalize_choices({ "on", "off" })
    end
    add(choices)
  end
  add(safe_choices(arg.suggest, parsed))
  return out
end

-- The candidates for the token the caret sits in within `text`, best first, and
-- the byte offsets `start, end` of the run they replace. The run is the token,
-- or the whole tail a greedy argument swallows, reaching to the end of the line;
-- in whitespace it is empty, at the caret. Matching is against the text before
-- the caret.
function Parser:complete(text, caret)
  text = text or ""
  caret = caret or #text
  if caret < 0 then
    caret = 0
  elseif caret > #text then
    caret = #text
  end
  local toks = tokenize_offsets(text)

  -- The token the caret sits in - its bytes span [start, end) with the caret at
  -- or between them - or none, in whitespace.
  local active = nil
  for i, tok in ipairs(toks) do
    local s = tok.start - 1
    if s <= caret and caret <= s + #tok.text then
      active = i
      break
    end
  end

  -- The run [rstart, rend) a suggestion replaces, and the prefix typed into it.
  local rstart, rend, prefix
  if active ~= nil then
    rstart = toks[active].start - 1
    rend = rstart + #toks[active].text
    prefix = toks[active].text:sub(1, caret - rstart)
  else
    rstart, rend, prefix = caret, caret, ""
  end

  -- Completing a flag by its dashes.
  if prefix:match("^%-") ~= nil then
    local out = {}
    for _, flag in ipairs(self.flags) do
      -- short and long are each optional, so gather them without a nil hole
      -- that ipairs would stop at.
      local spellings = { flag.short }
      spellings[#spellings + 1] = flag.long
      for _, spelling in ipairs(spellings) do
        if spelling:sub(1, #prefix) == prefix then
          out[#out + 1] = spelling
        end
      end
    end
    return out, rstart, rend
  end

  -- The positionals filled before the active one: non-flag tokens ending at or
  -- before the caret. Resolving them lets a later argument's choices depend on
  -- them.
  local consumed = {}
  for i, tok in ipairs(toks) do
    if i ~= active and self:flag_for(tok.text) == nil then
      if (tok.start - 1) + #tok.text <= caret then
        consumed[#consumed + 1] = tok
      end
    end
  end
  local slot = #consumed + 1

  -- A greedy argument, always last, owns everything from its slot to the end of
  -- the line.
  local greedy_idx = nil
  local last = self.positionals[#self.positionals]
  if last ~= nil and last.greedy then
    greedy_idx = #self.positionals
  end

  local arg, prior_count
  if greedy_idx ~= nil and slot >= greedy_idx then
    arg = last
    prior_count = greedy_idx - 1
    -- The run reaches from the greedy slot's first token - or the caret, if none
    -- has been typed there yet - to the end of the line.
    if consumed[greedy_idx] ~= nil then
      rstart = consumed[greedy_idx].start - 1
    elseif active == nil then
      rstart = caret
    end
    rend = #text
    prefix = text:sub(rstart + 1, caret)
  else
    arg = self.positionals[slot]
    prior_count = slot - 1
  end
  if arg == nil then
    return {}, rstart, rend
  end
  local values = {}
  for j = 1, prior_count do
    local prior = self.positionals[j]
    if prior ~= nil and consumed[j] ~= nil then
      values[prior.name] = resolve(prior, consumed[j].text, values)
    end
  end
  return candidates_for(arg, values, prefix), rstart, rend
end

-- The metavar an argument shows in the synopsis: its alternatives joined with
-- `|` when they name themselves, else its own.
local function arg_metavar(arg)
  local names = {}
  for _, matcher in ipairs(arg.matchers) do
    if matcher.metavar ~= nil then
      names[#names + 1] = matcher.metavar
    end
  end
  if #names > 0 then
    return table.concat(names, "|")
  end
  return arg.metavar
end

-- Whether an argument's alternatives describe themselves, earning a line each.
local function is_detailed(arg)
  for _, matcher in ipairs(arg.matchers) do
    if matcher.metavar ~= nil or matcher.help ~= nil then
      return true
    end
  end
  return false
end

function Parser:usage()
  local parts = { self.prog or "?" }
  for _, flag in ipairs(self.flags) do
    if flag.name ~= "help" then
      parts[#parts + 1] = "[" .. (flag.short or flag.long) .. "]"
    end
  end
  for _, arg in ipairs(self.positionals) do
    local mv = arg_metavar(arg)
    if arg.greedy then
      local inner = mv .. "..."
      parts[#parts + 1] = arg.optional and ("[" .. inner .. "]") or inner
    elseif arg.optional then
      parts[#parts + 1] = "[" .. mv .. "]"
    else
      parts[#parts + 1] = mv
    end
  end

  local lines = {
    trx.locale.format("console/argparse/usage", table.concat(parts, " ")),
  }
  -- metavar, then a "(...)" hint, then ": help". The help reads as a game-string
  -- key and falls back to itself.
  local function detail(metavar, hint, help)
    if metavar == nil then
      return
    end
    local line = "  " .. metavar
    if hint ~= nil then
      line = line .. " (" .. hint .. ")"
    end
    if help ~= nil then
      line = line .. ": " .. trx.locale.get(help)
    end
    lines[#lines + 1] = line
  end

  for _, arg in ipairs(self.positionals) do
    if is_detailed(arg) then
      -- Each alternative that names or describes itself gets its own line; the
      -- "(...)" says only the type, since the metavar already names the choice.
      for _, matcher in ipairs(arg.matchers) do
        if matcher.metavar ~= nil or matcher.help ~= nil then
          detail(
            matcher.metavar or arg.metavar,
            format_hint({ type = matcher.type }),
            matcher.help
          )
        end
      end
    else
      local hint = format_hint(hint_of(arg, {}))
      if arg.help ~= nil or hint ~= nil then
        detail(arg.metavar, hint, arg.help)
      end
    end
  end
  return table.concat(lines, "\n")
end

api.define("argparse.new", {
  description = "Creates an argument parser. See the module description for the parser's methods.",
  params = {
    {
      name = "spec",
      type = "table",
      optional = true,
      description = "`prog`: the command word, for messages. `description`: what the command does.",
    },
  },
  returns = {
    type = "table",
    description = "A parser. Describe its arguments with `positional`, `any_of`, `rest` and "
      .. "`flag`, then read them with `parse` or offer completions with `complete`.",
  },
  examples = {
    [[local p = trx.argparse.new({ prog = "weather" })
p:positional("state", { choices = { "snow", "rain", "none" } })
local parsed = p:parse("snow")  -- { state = "snow" }]],
  },
  impl = Parser.new,
})
