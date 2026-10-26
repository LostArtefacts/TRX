local raw = trxc.locale
local raw_events = trxc.events
local raw_catalog = trxc.catalog
local raw_strings = trxc.strings
local h = require("trx.internal.helpers")

require("trx.log")
require("trx.signal")

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

---The languages that the game has strings files for, by the codes that the
---`language` setting takes. <!--noref: language-->
---@return string[] # The codes.
---@type fun(): string[]
M.text_languages = raw.text_languages

---The dubs installed beside the retail files, by the codes that the
---`audio.media_language` setting takes. <!--noref: audio.media_language-->
---@return string[] # The codes.
---@type fun(): string[]
M.voice_languages = raw.voice_languages

---The name a language gives itself in its strings file.
---@param code string The language code, such as `fr`. <!--noref: fr-->
---@return string? # The name, or `nil` where no strings file names it.
---@type fun(code: string): string?
M.language_name = raw.language_name

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

local changed = trx.signal.new(0)
local make, resolve

local function same_args(a, b)
  if a.n ~= b.n then
    return false
  end
  for i = 1, a.n do
    if a[i] ~= b[i] then
      return false
    end
  end
  return true
end

local function bump()
  changed:set(changed:get() + 1)
end

raw_events.attach(h.constants("LUA_EVENT_TYPE").STRINGS_CHANGE, bump)

---Reloads the current language's text from disk.
---@return boolean # Whether the reload succeeded.
---@type fun(): boolean
M.reload = raw.reload

---Text that looks itself up again in the player's language, for anything that
---shows text on screen. A label that is given one shows it in the current
---language, and changes when the player picks another one.
---
---Text resolves when it is turned into a string, and keeps the result until
---the game text changes, so showing it again costs nothing. Text made from the
---same key and arguments is equal.
---
---```lua
---trx.ui.widgets.Label({ text = trx.locale.text("general/misc/off") })
---```
---@class (exact) trx.locale.Text
---@operator concat(string|trx.locale.Text): trx.locale.Text
---@operator len: integer
---@trx.operator tostring The text in the player's language.
---@trx.operator eq Two texts are equal when they are made from the same parts.
---@trx.operator concat Joins text into a new text, whichever side of the `..` it is on.
---@trx.operator len The length of the text in the player's language.
---@field private _key string?
---@field private _pattern string?
---@field private _args table
---@field private _source trx.locale.Text?
---@field private _fn function?
---@field private _resolver function?
---@field private _text string?
---@field private _at integer?
local Text = h.class("locale.Text", {
  operators = {
    tostring = function(self)
      return resolve(self)
    end,
    eq = function(a, b)
      return a._key == b._key
        and a._pattern == b._pattern
        and a._source == b._source
        and a._fn == b._fn
        and a._resolver == b._resolver
        and same_args(a._args, b._args)
    end,
    concat = function(a, b)
      return make({ _pattern = "%s%s", _args = table.pack(a, b) })
    end,
    len = function(self)
      return #resolve(self)
    end,
  },
})

function make(fields)
  local text = h.new(Text)
  for key, value in pairs(fields) do
    rawset(text, key, value)
  end
  return text
end

local function text_of(value)
  if getmetatable(value) == Text then
    return resolve(value)
  end
  return value
end

local function work_out(self)
  if self._resolver ~= nil then
    return self._resolver() or ""
  end
  if self._source ~= nil then
    return self._fn(resolve(self._source))
  end
  local pattern = self._pattern
  if self._key ~= nil then
    pattern = raw.get(self._key) or self._key
  end
  local args = self._args
  if args.n == 0 then
    return pattern
  end
  local values = {}
  for i = 1, args.n do
    values[i] = text_of(args[i])
  end
  local ok, formatted =
    pcall(string.format, pattern, table.unpack(values, 1, args.n))
  if not ok then
    trx.log.warn(
      ("locale.text(%q): %s"):format(self._key or pattern, formatted)
    )
    return pattern
  end
  return formatted
end

function resolve(self)
  local at = changed:get()
  if self._at ~= at then
    self._text = work_out(self)
    self._at = at
  end
  return self._text
end

-- The engine's case mapping knows the letters beyond ASCII that the game's
-- font draws, and leaves the text markup alone.
local function upper(text)
  return raw_strings ~= nil and raw_strings.upper(text) or text:upper()
end

local function lower(text)
  return raw_strings ~= nil and raw_strings.lower(text) or text:lower()
end

---The text in capitals, in the player's language, as `trx.strings.upper`
---turns it.
---@return trx.locale.Text # The text in capitals.
function Text:upper()
  return make({ _source = self, _fn = upper, _args = { n = 0 } })
end

---The text in small letters, in the player's language, as
---`trx.strings.lower` turns it.
---@return trx.locale.Text # The text in small letters.
function Text:lower()
  return make({ _source = self, _fn = lower, _args = { n = 0 } })
end

-- Text without arguments, by key, so that asking for the same key again
-- builds nothing.
local plain = {}

---The text behind a game string key, as text that follows the player's
---language. Use it for text that goes on screen, and `trx.locale.get` for
---text that a script reads.
---
---```lua
---local hint = trx.locale.text("general/misc/pagination_nav", 1, 5)
---```
---@param key string The key, e.g. `general/misc/off`.
---@param ... any What to fill the placeholders with. A `trx.locale.Text` among them follows the player's language too.
---@return trx.locale.Text # The text.
function M.text(key, ...)
  local count = select("#", ...)
  if count == 0 then
    local found = plain[key]
    if found == nil then
      found = make({ _key = key, _args = { n = 0 } })
      plain[key] = found
    end
    return found
  end
  return make({ _key = key, _args = table.pack(...) })
end

local object_names = {}
local OBJECTS = h.constants("CATALOG_CONTEXT").OBJECTS

---The name the game shows for an object, as text that follows the player's
---language.
---@param object trx.catalog.objects The object.
---@return trx.locale.Text # The name, which is empty where the object has none.
function M.object_name(object)
  local found = object_names[object]
  if found == nil then
    found = make({
      _resolver = function()
        local key = raw_catalog.key(OBJECTS, object)
        local names = key ~= nil and raw.get("objects/" .. key .. "/name")
        return names and names:match("^[^|]*") or nil
      end,
      _args = { n = 0 },
    })
    object_names[object] = found
  end
  return found
end

local level_titles = {}

---The name the game shows for a level, as text that follows the player's
---language.
---@param level trx.game.Level The level.
---@return trx.locale.Text # The name.
function M.level_title(level)
  local key = ("%s/%d"):format(level.type, level.num)
  local found = level_titles[key]
  if found == nil then
    found = make({
      _resolver = function()
        return level.title
      end,
      _args = { n = 0 },
    })
    level_titles[key] = found
  end
  return found
end

---The signals the text speaks through, for a script that would rather hear
---about a change than ask after one.
---@class (exact) trx.locale.signals
---@trx.readonly changed
---@field changed trx.signal.Signal Says when the game text changes, such as when the player picks another language or a level brings text of its own. It holds a count that goes up with each change.
M.signals = h.namespace("locale.signals")

h.properties(M.signals, "locale.signals", {
  changed = {
    get = function()
      return changed
    end,
  },
})
