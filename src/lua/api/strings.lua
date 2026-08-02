local raw = trxc.strings
local api = trx.api

api.module("strings", {
  order = 28,
  description = "Utilities for working with strings.\n\n"
    .. "Not to be confused with `trx.locale`, which is the text a player reads: this module is "
    .. "about manipulating strings, that one is about which string the player gets.",
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
    type = "table",
    description = "The matches, best first.",
    list = true,
    fields = {
      {
        name = "key",
        type = "string",
        description = "The candidate that matched.",
      },
      {
        name = "value",
        type = "any",
        description = "What the candidate carried.",
      },
      {
        name = "score",
        type = "number",
        description = "How well it matched.",
      },
      {
        name = "is_full",
        type = "boolean",
        description = "Whether the whole candidate matched.",
      },
      {
        name = "is_word",
        type = "boolean",
        description = "Whether a whole word matched.",
      },
    },
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
    { name = "numbers", type = "table", description = "List of integers." },
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
