local raw = trxc.strings
local api = trx.api

api.module("strings", {
  order = 19,
  description = "Utilities for working with strings.\n\n"
    .. "Not to be confused with `trx.locale`, which is the text a player reads: this module is "
    .. "about manipulating strings, that one is about which string the player gets.",
})

api.define("strings.fuzzy_match", {
  description = "Matches what someone typed against a list of candidates, forgivingly: `big medi` "
    .. "finds `large medipack`.\n\n"
    .. "Candidates are ranked, best first. Each carries a `value` of the caller's choosing, which "
    .. "comes back untouched on the match - hang an id off it and read it back.",
  params = {
    { name = "input", type = "string", description = "What the player typed." },
    {
      name = "sources",
      type = "table",
      description = "List of `{ key = <the name>, value = <anything>, weight = <integer> }`. "
        .. "The key is a non-empty string. A heavier candidate wins a tie; weight defaults to 1, "
        .. "and a weight of zero or less drops the candidate.",
    },
  },
  returns = {
    type = "table",
    description = "The matches, best first: `{ key, value, score, is_full, is_word }`. `is_full` "
      .. "means the whole candidate matched, `is_word` that a whole word did.",
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

api.define("strings.regex_match", {
  description = "Whether a subject matches a regular expression. Case-insensitive.",
  params = {
    { name = "subject", type = "string" },
    { name = "pattern", type = "string", description = "A PCRE regular expression." },
  },
  returns = { type = "boolean" },
  examples = { [[if trx.strings.regex_match(args, "^\\d+$") then ... end]] },
  impl = raw.regex_match,
})
