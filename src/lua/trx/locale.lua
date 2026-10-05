local raw = trxc.locale
local h = require("trx.internal.helpers")

require("trx.log")

---@class (partial) trx
---@field locale trx.locale

---The text the player reads, in the player's own language.
---@trx.module 24
---@class (exact) trx.locale
local M = h.module("locale")

---Declares game string keys and the text behind them.
---
---A key belongs with the script that shows it, so a command carries its own
---wording. What is declared here is a fallback, and it takes only for a key
---nothing else holds: the strings files, their translations, and any earlier
---declaration keep the text they already carry.
---
---```lua
---trx.locale.declare({
---  ["console/cmd/heal/help"] = "Heals Lara back to full health.",
---  ["console/cmd/heal/success"] = "Healed Lara back to full health",
---})
---```
---@param strings table Keys to their English text.
function M.declare(strings)
  assert(type(strings) == "table", "trx.locale.declare expects a table")
  for key, text in pairs(strings) do
    assert(type(key) == "string", "trx.locale.declare: key must be a string")
    assert(type(text) == "string", "trx.locale.declare: text must be a string")
    raw.declare(key, text)
  end
end

---The text behind a game string key.
---
---```lua
---trx.console.log(trx.locale.get("general/misc/off"))
---```
---@param key string The key, e.g. `general/misc/off`.
---@return string # The key itself if nothing is behind it, so a typo shows up on screen rather than as a nil further down.
function M.get(key)
  return raw.get(key) or key
end

---The text behind a key with its placeholders filled in.
---
---```lua
---trx.console.log(trx.locale.format("general/misc/pagination_nav", 1, 5))
---```
---@param key string The key, e.g. `general/misc/off`.
---@param ... any What to fill the placeholders with.
---@return string # The text with the arguments in it. A translation whose placeholders do not line up with the arguments comes back unformatted, with a warning in the log: a player is better served by text they can read than by a script that stops.
function M.format(key, ...)
  local text = trx.locale.get(key)
  local ok, formatted = pcall(string.format, text, ...)
  if not ok then
    trx.log.warn(("locale.format(%q): %s"):format(key, formatted))
    return text
  end
  return formatted
end

---Reloads the current language's text from disk.
---@return boolean # Whether the reload succeeded.
---@type fun(): boolean
M.reload = raw.reload
