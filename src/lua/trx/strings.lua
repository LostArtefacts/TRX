local raw = trxc.strings
local h = require("trx.internal.helpers")

---@class trx
---@field strings trx.strings

---Utilities for working with strings.
---
---Not to be confused with `trx.locale`, which is the text a player reads: this
---module is about manipulating strings, that one is about which string the
---player gets.
---@trx.module 34
---@class (exact) trx.strings
local M = h.module("strings")

---A candidate that matched, and how well.
---@trx.record
---@class trx.strings.Match
---@field key string The candidate that matched.
---@field value? any What the candidate carried, where it carried one.
---@field score number How well it matched.
---@field is_full boolean Whether the whole candidate matched.
---@field is_word boolean Whether a whole word matched.

---@class (exact) trx.strings.fuzzy_match.sources
---@field key string The name to match against. Non-empty.
---@field value any Anything of the caller's, handed back on the match.
---@field weight? integer A heavier candidate wins a tie. Zero or less drops it.
---@trx.default weight 1

---Matches what someone typed against a list of candidates, forgivingly: `big
---medi` finds `large medipack`.
---
---Candidates are ranked, best first. Each carries a
---`trx.strings.fuzzy_match.sources.value` of the caller's choosing, which comes
---back untouched on the match - hang an id off it and read it back.
---
---```lua
---local matches = trx.strings.fuzzy_match("wolf", {
---  { key = "wolf", value = trx.catalog.objects.WOLF },
---  { key = "bear", value = trx.catalog.objects.BEAR },
---})
---local best = matches[1]
---```
---@param input string What the player typed.
---@param sources trx.strings.fuzzy_match.sources[] The candidates.
---@return trx.strings.Match[] # The best match comes first.
---@type fun(input: string, sources: trx.strings.fuzzy_match.sources[]): trx.strings.Match[]
M.fuzzy_match = raw.fuzzy_match

---Reads a boolean the way the console does: `1`, `true` or `on` for true, `0`,
---`false` or `off` for false, in any case. Anything else is not a boolean.
---<!--noref: on, off-->
---
---```lua
---local on = trx.strings.parse_bool("on")
---```
---@param text string The text to read.
---@return boolean? # `nil` when the text does not name a boolean.
function M.parse_bool(text)
  local lowered = text:lower()
  if lowered == "1" or lowered == "true" or lowered == "on" then
    return true
  end
  if lowered == "0" or lowered == "false" or lowered == "off" then
    return false
  end
  return nil
end

---Writes a list of whole numbers as ranges, so that a long run reads as one:
---`{ 0, 2, 3, 4, 9 }` becomes `0, 2-4, 9`.
---
---The list is sorted first, and duplicates survive as they are, so the caller
---need not tidy up before handing it over.
---
---```lua
---trx.strings.collapse_ranges({ 4, 1, 2, 3 }) -- "1-4"
---```
---@param numbers integer[] The numbers to write out.
---@param separator? string What to put between the parts. Defaults to `", "`.
---@return string # Empty when the list is.
function M.collapse_ranges(numbers, separator)
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
end

---Whether a subject matches a regular expression. Case-insensitive.
---
---```lua
---if trx.strings.regex_match(args, "^\\d+$") then ... end
---```
---@param subject string The text to search.
---@param pattern string A PCRE regular expression.
---@return boolean # True where the pattern matches anywhere in the subject.
---@type fun(subject: string, pattern: string): boolean
M.regex_match = raw.regex_match

---Spells a name the way the console shows one: lower case, with underscores
---read as dashes. This is how an enum constant is offered for completion, and a
---catalog name resolves in either spelling.
---
---```lua
---trx.strings.dash_case("LARA_NO") -- "lara-no"
---```
---@param text string The name to spell.
---@return string # The name in dashed lower case.
function M.dash_case(text)
  return (text:lower():gsub("_", "-"))
end

---Takes the shared indentation off a block of text, so that a long string
---written inside `[[ ]]` reads as what it says rather than as where it sat in
---the file. Leading and trailing blank lines go too.
---
---The deepest lines keep the rest of their indentation, since a block may lay
---something out, and four spaces of it is a code block in markdown. Text may
---open on the line the brackets are on, and that line then sets nothing and
---keeps what it has, however the ones under it are written.
---
---```lua
---local help = trx.strings.dedent([[
---      Usage: /give <what>
---        keys   every plot item the level has a place for
---    ]])
---```
---@param text string The text to take in.
---@return string # The text at the left margin.
function M.dedent(text)
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
end
