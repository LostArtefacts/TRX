local raw = trxc.locale
local api = trx.api

require("trx.log")

api.module("locale", {
  order = 22,
  description = "The text the player reads, in the player's own language.",
})

api.define("locale.declare", {
  description = [[
Declares game string keys and the text behind them.

A key belongs with the script that shows it, so a command carries its own
wording. What is declared here is a fallback, and it takes only for a key
nothing else holds: the strings files, their translations, and any earlier
declaration keep the text they already carry.]],
  params = {
    {
      name = "strings",
      type = "table",
      description = "Keys to their English text.",
    },
  },
  examples = {
    [[trx.locale.declare({
  ["console/cmd/heal/help"] = "Heals Lara back to full health.",
  ["console/cmd/heal/success"] = "Healed Lara back to full health",
})]],
  },
  impl = function(strings)
    assert(type(strings) == "table", "trx.locale.declare expects a table")
    for key, text in pairs(strings) do
      assert(type(key) == "string", "trx.locale.declare: key must be a string")
      assert(
        type(text) == "string",
        "trx.locale.declare: text must be a string"
      )
      raw.declare(key, text)
    end
  end,
})

api.define("locale.get", {
  description = "The text behind a game string key.",
  params = {
    {
      name = "key",
      type = "string",
      description = "The key, e.g. `general/misc/off`.",
    },
  },
  returns = {
    type = "string",
    description = "The key itself if nothing is behind it, so a typo shows up on screen rather "
      .. "than as a nil further down.",
  },
  examples = { [[trx.console.log(trx.locale.get("general/misc/off"))]] },
  impl = function(key)
    return raw.get(key) or key
  end,
})

api.define("locale.format", {
  description = "The text behind a key with its placeholders filled in.",
  params = {
    {
      name = "key",
      type = "string",
      description = "The key, e.g. `general/misc/off`.",
    },
    {
      name = "...",
      type = "any",
      description = "What to fill the placeholders with.",
    },
  },
  returns = {
    type = "string",
    description = "The text with the arguments in it. A translation whose placeholders do not "
      .. "line up with the arguments comes back unformatted, with a warning in the log: a "
      .. "player is better served by text they can read than by a script that stops.",
  },
  examples = {
    [[trx.console.log(trx.locale.format("general/misc/pagination_nav", 1, 5))]],
  },
  impl = function(key, ...)
    local text = trx.locale.get(key)
    local ok, formatted = pcall(string.format, text, ...)
    if not ok then
      trx.log.warn(("locale.format(%q): %s"):format(key, formatted))
      return text
    end
    return formatted
  end,
})

api.define("locale.reload", {
  description = "Reloads the current language's text from disk.",
  returns = { type = "boolean", description = "Whether the reload succeeded." },
  impl = raw.reload,
})
