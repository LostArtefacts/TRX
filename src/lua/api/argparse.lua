local api = trx.api

require("trx.locale")

api.module("argparse", {
  order = 24,
  description = [[
A small, declarative argument parser for console commands, in the shape of
Python's argparse.

A parser both reads a command's arguments and offers completions for them,
from one declaration. Every command written with `trx.console.register` has
one; a command shapes it through the `trx.console.register.spec.args` function
it hands over, and `trx.console.register.spec.run` then receives a table of
parsed values. A command that shapes nothing takes no arguments, and is told so
when given one.

Every parser answers `-h` and `--help` on its own, printing what it accepts.

How a positional reads a token is its matcher: a type to coerce to, a set of
choices, or a function of its own. These do not combine on one positional; a
value that is a number *or* a name is two matchers, declared with
`trx.argparse.Parser:any_of`.

Positionals are read in order, and an optional one a token does not fit is
passed over: the token goes to the next positional, and the one skipped stays
nil. That is what lets a command take a leading argument it can also be used
without - a verb before a value, a count before a name. Completion follows the
same path, so a slot offers what every argument reachable from it takes. A
token nothing takes is reported against the first argument that refused it.

A choice is either a bare string, where the key and value are the same, or a
`{ key, value }` pair, where the key is matched and shown and the value is what
`trx.argparse.Parser:parse` gives back. Matching is forgiving, through
`trx.strings.fuzzy_match`.]],
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

-- The class is the declaration's, further down: what a script may call on a
-- parser is what api.type names, and these are the bodies behind it. The name
-- is taken here, because the declaration's own methods hand a parser back.
local Parser
local P = {}

function P.new(spec)
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

function P:positional(name, opts)
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

function P:any_of(name, alternatives, opts)
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
function P:rest(name, opts)
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

function P:flag(name, opts)
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

function P:flag_for(token)
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

-- Which positional a token fills, starting at `pos_idx`: that one, or - when it
-- is optional and no matcher of its takes the token - the first one after it
-- that does. An argument passed over this way is left unfilled, which is what
-- lets a command take a leading argument it can also be given without.
--
-- Returns the positional's index, the argument, and the value it read. A token
-- nothing takes returns nil and the error from the first argument that refused
-- it, since that is the one the player meant to fill.
local function take(parser, pos_idx, tok, args, values)
  local first_err = nil
  for idx = pos_idx, #parser.positionals do
    local arg = parser.positionals[idx]
    -- A greedy argument reads the rest of the line as one token; the plain
    -- token otherwise.
    local token = arg.greedy and args:sub(tok.start) or tok.text
    local value, ok = resolve(arg, token, values)
    if ok then
      return idx, arg, value
    end
    if first_err == nil then
      first_err = {
        kind = "invalid",
        metavar = arg.metavar,
        token = token,
        hint = hint_of(arg, values),
      }
    end
    -- A required argument has to take this token, so there is nothing to pass
    -- it on to.
    if not arg.optional then
      break
    end
  end
  return nil, nil, nil, first_err
end

function P:parse(args)
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
    local flag = P.flag_for(self, tok.text)
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
      if self.positionals[pos_idx] == nil then
        return nil, { kind = "unexpected", token = tok.text }
      end
      local idx, arg, value, err = take(self, pos_idx, tok, args, values)
      if arg == nil then
        return nil, err
      end
      values[arg.name] = value
      pos_idx = idx + 1
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
function P:format_error(err)
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

-- Puts the keys in alphabetical order for the completion list. The keys that
-- start with the typed text come first, and the `-` that puts the default back
-- comes last. Upper case and lower case letters have the same order, and `Rain`
-- stays together with `rain`.
local function sort_candidates(keys, active)
  local prefix = active:lower()
  local function has_prefix(key)
    return key:lower():sub(1, #prefix) == prefix
  end
  table.sort(keys, function(a, b)
    if has_prefix(a) ~= has_prefix(b) then
      return has_prefix(a)
    end
    if (a == "-") ~= (b == "-") then
      return b == "-"
    end
    if a:lower() ~= b:lower() then
      return a:lower() < b:lower()
    end
    return a < b
  end)
  return keys
end

-- The keys an argument offers for `active`, in alphabetical order. An empty
-- `active` offers them all. Both the choices its matchers restrict to and its
-- `suggest` list contribute; a boolean offers on/off without being told to.
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
  return sort_candidates(out, active)
end

-- The candidates for the token the caret sits in within `text`, best first, and
-- the byte offsets `start, end` of the run they replace. The run is the token,
-- or the whole tail a greedy argument swallows, reaching to the end of the line;
-- in whitespace it is empty, at the caret. Matching is against the text before
-- the caret.
function P:complete(text, caret)
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
    return sort_candidates(out, prefix), rstart, rend
  end

  -- The positionals filled before the active one: non-flag tokens ending at or
  -- before the caret. Resolving them lets a later argument's choices depend on
  -- them.
  local consumed = {}
  for i, tok in ipairs(toks) do
    if i ~= active and P.flag_for(self, tok.text) == nil then
      if (tok.start - 1) + #tok.text <= caret then
        consumed[#consumed + 1] = tok
      end
    end
  end

  -- Where those tokens landed, walking as parse walks, so an optional argument
  -- one of them passed over does not shift the slots behind it. `slot` is the
  -- positional the active token fills, and `values` what the earlier ones read.
  local values = {}
  local slot = 1
  -- The first token to land on the greedy argument, which is where the run it
  -- owns begins. Read off the walk rather than counted, since a token that
  -- passed an argument over fills a slot its own position does not name.
  local greedy_tok = nil
  for _, tok in ipairs(consumed) do
    local idx, arg, value = take(self, slot, tok, text, values)
    if arg == nil then
      -- A token nothing takes is one the player is still fixing. Move on, so
      -- what follows it still completes.
      slot = slot + 1
    else
      if arg.greedy and greedy_tok == nil then
        greedy_tok = tok
      end
      values[arg.name] = value
      slot = idx + 1
    end
  end

  -- A greedy argument, always last, owns everything from its slot to the end of
  -- the line.
  local greedy_idx = nil
  local last = self.positionals[#self.positionals]
  if last ~= nil and last.greedy then
    greedy_idx = #self.positionals
  end

  if greedy_idx ~= nil and slot >= greedy_idx then
    -- The run reaches from the greedy slot's first token - or the caret, if none
    -- has been typed there yet - to the end of the line.
    if greedy_tok ~= nil then
      rstart = greedy_tok.start - 1
    elseif active == nil then
      rstart = caret
    end
    rend = #text
    prefix = text:sub(rstart + 1, caret)
    return candidates_for(last, values, prefix), rstart, rend
  end

  -- What the slot takes: the argument sitting there, and - while that one is
  -- optional - the ones a token could pass over to. Reaching a greedy one that
  -- way makes the run the whole tail, since that is what it would swallow.
  local out = {}
  local seen = {}
  for idx = slot, #self.positionals do
    local arg = self.positionals[idx]
    for _, candidate in ipairs(candidates_for(arg, values, prefix)) do
      if not seen[candidate] then
        seen[candidate] = true
        out[#out + 1] = candidate
      end
    end
    if arg.greedy then
      rend = #text
      break
    end
    if not arg.optional then
      break
    end
  end
  return out, rstart, rend
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

function P:usage()
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

-- The options every positional takes, whichever way it reads its token.
local ARG_OPTS = {
  {
    name = "optional",
    type = "boolean",
    optional = true,
    description = "Lets the argument be left out.",
  },
  {
    name = "greedy",
    type = "boolean",
    optional = true,
    description = "Reads the rest of the line as one token, so a value with spaces in it "
      .. "still arrives whole.",
  },
  {
    name = "metavar",
    type = "string",
    optional = true,
    description = "What the argument is called in messages and in the synopsis. Its own name "
      .. "by default.",
  },
  {
    name = "suggest",
    type = "function",
    optional = true,
    description = "Completions to offer, without restricting what is accepted or being shown "
      .. "in errors. For a free value with a long list behind it, like a setting name.",
  },
  {
    name = "help",
    type = "string",
    optional = true,
    description = "What the argument is for, shown in the help.",
  },
}

-- A matcher and the options around it, which is what a single-matcher argument
-- declares in one table.
local function positional_opts()
  local out = {
    {
      name = "type",
      type = "string",
      optional = true,
      description = 'Coerce the token: `"integer"`, `"number"`, `"string"` or `"boolean"`.',
    },
    {
      name = "choices",
      type = "any",
      optional = true,
      description = "The allowed set: a list of values, or a function of the values parsed so "
        .. "far returning one. The token must match one; the set is shown in errors and "
        .. "completes.",
    },
    {
      name = "match",
      type = "function",
      optional = true,
      description = "A function of the token and the values parsed so far, returning the "
        .. "value and whether it took, for a shape of its own.",
    },
  }
  for _, opt in ipairs(ARG_OPTS) do
    out[#out + 1] = opt
  end
  return out
end

Parser = api.type("argparse.Parser", {
  description = "An argument parser, built up a call at a time. Every method hands the parser "
    .. "back, so the calls chain.",

  methods = {
    positional = {
      description = "Adds a positional argument, read one way.",
      params = {
        {
          name = "name",
          type = "string",
          description = "What the parsed value is keyed by.",
        },
        {
          name = "opts",
          type = "table",
          optional = true,
          description = "How it reads its token, and how it behaves. It reads a token one "
            .. "way: name at most one of `trx.argparse.Parser.positional.opts.type`, "
            .. "`trx.argparse.Parser.positional.opts.choices` and "
            .. "`trx.argparse.Parser.positional.opts.match`, and use "
            .. "`trx.argparse.Parser:any_of` for several.",
          fields = positional_opts(),
        },
      },
      returns = {
        type = "argparse.Parser",
        description = "The same parser, so declarations chain.",
      },
      impl = P.positional,
    },

    any_of = {
      description = "Adds a positional whose value is the first of several matchers to take "
        .. "the token, for an argument that is a number or a name.",
      params = {
        {
          name = "name",
          type = "string",
          description = "What the parsed value is keyed by.",
        },
        {
          name = "alternatives",
          type = "table",
          list = true,
          description = "The ways the token may read, tried in order.",
          fields = {
            {
              name = "type",
              type = "string",
              optional = true,
              description = "As for `trx.argparse.Parser:positional`.",
            },
            {
              name = "choices",
              type = "any",
              optional = true,
              description = "As for `trx.argparse.Parser:positional`.",
            },
            {
              name = "match",
              type = "function",
              optional = true,
              description = "As for `trx.argparse.Parser:positional`.",
            },
            {
              name = "metavar",
              type = "string",
              optional = true,
              description = "Names this alternative, which earns it a line of its own in the "
                .. "help.",
            },
            {
              name = "help",
              type = "string",
              optional = true,
              description = "What this alternative is for.",
            },
          },
        },
        {
          name = "opts",
          type = "table",
          optional = true,
          description = "How the argument behaves.",
          fields = ARG_OPTS,
        },
      },
      returns = {
        type = "argparse.Parser",
        description = "The same parser, so declarations chain.",
      },
      impl = P.any_of,
    },

    rest = {
      description = "Adds an argument taking the rest of the line from here on, verbatim as "
        .. "one string, or `nil` where an optional one is absent. Always the last argument.",
      params = {
        {
          name = "name",
          type = "string",
          description = "What the parsed value is keyed by.",
        },
        {
          name = "opts",
          type = "table",
          optional = true,
          description = "How the argument behaves.",
          fields = ARG_OPTS,
        },
      },
      returns = {
        type = "argparse.Parser",
        description = "The same parser, so declarations chain.",
      },
      impl = P.rest,
    },

    flag = {
      description = "Adds a boolean flag, which may sit anywhere in the line.",
      params = {
        {
          name = "name",
          type = "string",
          description = "What the parsed value is keyed by. `help` is reserved. <!--noref: help-->",
        },
        {
          name = "opts",
          type = "table",
          optional = true,
          description = "How it is spelled and what it is for.",
          fields = {
            {
              name = "short",
              type = "string",
              optional = true,
              description = 'The short spelling, such as `"-f"`.',
            },
            {
              name = "long",
              type = "string",
              optional = true,
              description = 'The long spelling, such as `"--force"`.',
            },
            {
              name = "help",
              type = "string",
              optional = true,
              description = "What the flag is for, shown in the help.",
            },
          },
        },
      },
      returns = {
        type = "argparse.Parser",
        description = "The same parser, so declarations chain.",
      },
      impl = P.flag,
    },

    parse = {
      description = "Reads an argument line. A value carried by a `{ key, value }` choice "
        .. "comes back as its value, and `-h`/`--help` comes back as `{ help = true }`.",
      params = {
        {
          name = "args",
          type = "string",
          optional = true,
          description = "The line as the player typed it.",
        },
      },
      returns = {
        {
          type = "table",
          nullable = true,
          description = "The values, keyed by argument name, or `nil` where the line was "
            .. "refused.",
        },
        {
          type = "table",
          nullable = true,
          description = "What was wrong, for `trx.argparse.Parser:format_error` to put into "
            .. "words.",
        },
      },
      impl = P.parse,
    },

    format_error = {
      description = "Turns what a refused line reported into a localized line naming what was "
        .. "wrong and what was expected.",
      params = {
        {
          name = "err",
          type = "table",
          description = "What `trx.argparse.Parser:parse` handed back.",
        },
      },
      returns = { type = "string", description = "The line, ready to print." },
      impl = P.format_error,
    },

    complete = {
      description = "The candidate completions for the token the caret sits in. Matching is "
        .. "against the text before the caret.",
      params = {
        {
          name = "text",
          type = "string",
          optional = true,
          description = "The line so far.",
        },
        {
          name = "caret",
          type = "integer",
          optional = true,
          description = "Where the caret sits, as a byte offset. The end of the line by "
            .. "default.",
        },
      },
      returns = {
        {
          type = "string",
          list = true,
          description = "The best match comes first.",
        },
        {
          type = "integer",
          description = "Where the run they replace starts. The run is the token, or the "
            .. "whole tail a greedy argument swallows; in whitespace it is empty, at the "
            .. "caret.",
        },
        { type = "integer", description = "Where that run ends." },
      },
      impl = P.complete,
    },

    usage = {
      description = "A short description of what the command accepts.",
      returns = {
        type = "string",
        description = "The synopsis, and a line per argument.",
      },
      impl = P.usage,
    },
  },
})

api.define("argparse.new", {
  description = "Creates an argument parser.",
  params = {
    {
      name = "spec",
      type = "table",
      optional = true,
      description = "What the parser calls itself in messages.",
      fields = {
        {
          name = "prog",
          type = "string",
          optional = true,
          description = "The command word.",
        },
        {
          name = "description",
          type = "string",
          optional = true,
          description = "What the command does.",
        },
      },
    },
  },
  returns = {
    type = "argparse.Parser",
    description = "A parser, with no arguments declared on it yet.",
  },
  examples = {
    [[local p = trx.argparse.new({ prog = "weather" })
p:positional("state", { choices = { "snow", "rain", "none" } })
local parsed = p:parse("snow")  -- { state = "snow" }]],
  },
  impl = P.new,
})
