local raw = trxc.strings
local api = trx.api

api.module("strings", {
  order = 32,
  description = "Utilities for working with strings.\n\n"
    .. "Not to be confused with `trx.locale`, which is the text a player reads: this module is "
    .. "about manipulating strings, that one is about which string the player gets.",
})

api.type("strings.Match", {
  record = true,
  description = "A candidate that matched, and how well.",
  fields = {
    key = { type = "string", description = "The candidate that matched." },
    value = {
      type = "any",
      optional = true,
      description = "What the candidate carried, where it carried one.",
    },
    score = { type = "number", description = "How well it matched." },
    is_full = {
      type = "boolean",
      description = "Whether the whole candidate matched.",
    },
    is_word = {
      type = "boolean",
      description = "Whether a whole word matched.",
    },
  },
})

api.define("strings.fuzzy_match", {
  description = "Matches what someone typed against a list of candidates, forgivingly: `big medi` "
    .. "finds `large medipack`.\n\n"
    .. "Candidates are ranked, best first. Each carries a "
    .. "`trx.strings.fuzzy_match.sources.value` of the caller's choosing, which comes back "
    .. "untouched on the match - hang an id off it and read it back.",
  params = {
    {
      name = "input",
      type = "string",
      description = "What the player typed.",
    },
    {
      name = "sources",
      type = "table",
      description = "The candidates.",
      list = true,
      fields = {
        {
          name = "key",
          type = "string",
          description = "The name to match against. Non-empty.",
        },
        {
          name = "value",
          type = "any",
          description = "Anything of the caller's, handed back on the match.",
        },
        {
          name = "weight",
          type = "integer",
          optional = true,
          default = 1,
          description = "A heavier candidate wins a tie. Zero or less drops it.",
        },
      },
    },
  },
  returns = {
    type = "strings.Match",
    list = true,
    description = "The best match comes first.",
  },
  examples = {
    [==[local matches = trx.strings.fuzzy_match("wolf", {
  { key = "wolf", value = trx.catalog.objects.WOLF },
  { key = "bear", value = trx.catalog.objects.BEAR },
})
local best = matches[1]]==],
  },
  impl = raw.fuzzy_match,
})

api.define("strings.parse_bool", {
  description = "Reads a boolean the way the console does: `1`, `true` or `on` for true, `0`, "
    .. "`false` or `off` for false, in any case. Anything else is not a boolean. "
    .. "<!--noref: on, off-->",
  params = {
    { name = "text", type = "string", description = "The text to read." },
  },
  returns = {
    type = "boolean",
    nullable = true,
    description = "`nil` when the text does not name a boolean.",
  },
  examples = { [[local on = trx.strings.parse_bool("on")]] },
  impl = function(text)
    local lowered = text:lower()
    if lowered == "1" or lowered == "true" or lowered == "on" then
      return true
    end
    if lowered == "0" or lowered == "false" or lowered == "off" then
      return false
    end
    return nil
  end,
})

api.define("strings.collapse_ranges", {
  description = "Writes a list of whole numbers as ranges, so that a long run reads as one: "
    .. "`{ 0, 2, 3, 4, 9 }` becomes `0, 2-4, 9`.\n\n"
    .. "The list is sorted first, and duplicates survive as they are, so the caller need not tidy "
    .. "up before handing it over.",
  params = {
    {
      name = "numbers",
      type = "integer",
      list = true,
      description = "The numbers to write out.",
    },
    {
      name = "separator",
      type = "string",
      optional = true,
      description = 'What to put between the parts. Defaults to `", "`.',
    },
  },
  returns = { type = "string", description = "Empty when the list is." },
  examples = {
    [[trx.strings.collapse_ranges({ 4, 1, 2, 3 }) -- "1-4"]],
  },
  impl = function(numbers, separator)
    local sorted = { table.unpack(numbers) }
    table.sort(sorted)

    local parts, i = {}, 1
    while i <= #sorted do
      local first = i
      while i < #sorted and sorted[i + 1] == sorted[i] + 1 do
        i = i + 1
      end
      if i > first then
        parts[#parts + 1] = ("%d-%d"):format(sorted[first], sorted[i])
      else
        parts[#parts + 1] = tostring(sorted[first])
      end
      i = i + 1
    end
    return table.concat(parts, separator or ", ")
  end,
})

api.define("strings.regex_match", {
  description = "Whether a subject matches a regular expression. Case-insensitive.",
  params = {
    {
      name = "subject",
      type = "string",
      description = "The text to search.",
    },
    {
      name = "pattern",
      type = "string",
      description = "A PCRE regular expression.",
    },
  },
  returns = {
    type = "boolean",
    description = "True where the pattern matches anywhere in the subject.",
  },
  examples = { [[if trx.strings.regex_match(args, "^\\d+$") then ... end]] },
  impl = raw.regex_match,
})

api.define("strings.dedent", {
  description = [=[
    Takes the shared indentation off a block of text, so that a long string
    written inside `[[ ]]` reads as what it says rather than as where it sat in
    the file. Leading and trailing blank lines go too.

    The deepest lines keep the rest of their indentation, since a block may lay
    something out, and four spaces of it is a code block in markdown. Text may
    open on the line the brackets are on, and that line then sets nothing and
    keeps what it has, however the ones under it are written.
  ]=],
  params = {
    { name = "text", type = "string", description = "The text to take in." },
  },
  returns = { type = "string", description = "The text at the left margin." },
  examples = {
    [==[local help = trx.strings.dedent([[
      Usage: /give <what>
        keys   every plot item the level has a place for
    ]])]==],
  },
  impl = function(text)
    local lines = {}
    for line in (text .. "\n"):gmatch("([^\n]*)\n") do
      lines[#lines + 1] = line
    end
    -- Lua drops the newline that follows `[[`, so what says which of the two
    -- spellings was written is whether the first line is indented at all: one
    -- written against the brackets is not.
    local from = text:match("^[ \t]") == nil and 2 or 1

    local shared
    for i = from, #lines do
      if lines[i]:match("%S") ~= nil then
        local indent = #lines[i]:match("^[ \t]*")
        if shared == nil or indent < shared then
          shared = indent
        end
      end
    end

    if shared ~= nil and shared > 0 then
      for i = from, #lines do
        lines[i] = lines[i]:sub(shared + 1)
      end
    end
    return (table.concat(lines, "\n"):gsub("^%s*\n", ""):gsub("%s+$", ""))
  end,
})
