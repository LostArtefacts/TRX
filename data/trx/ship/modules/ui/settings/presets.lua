-- The config presets: named sets of settings, each in a file of its own under
-- the presets directory, that the player applies all at once.
--
-- A preset file holds the game string of its name and the settings it sets:
--   { "name_gs": "dynamic/config_presets/tr1_pc", "config": { "key": value } }
--
-- Usage:
--   local presets = require("common.ui.settings.presets")
--   for _, preset in ipairs(presets.load()) do ... end

local rows = require("common.ui.settings.rows")

local M = {}

---@class common.ui.settings.Preset
---@field title string The name, in the language the player had as the presets loaded.
---@field name_gs? string The game string of the name.
---@field settings { key: string, value: any }[] The settings it sets.

---@class common.ui.settings.PresetChange
---@field key string The setting.
---@field current string The value it holds, as the settings menu shows it.
---@field target string The value the preset gives it, shown the same way.
---@field value any The value as the preset file holds it.

-- Reads one preset file. A file without a name or settings is not a preset.
local function read(path)
  local ok, data = pcall(trx.json.read_file, path)
  if not ok or type(data) ~= "table" then
    trx.log.warn(("cannot read the preset %s"):format(tostring(path)))
    return nil
  end
  if type(data.name_gs) ~= "string" or type(data.config) ~= "table" then
    trx.log.warn(
      ("the preset %s has no name or settings"):format(tostring(path))
    )
    return nil
  end
  local settings = {}
  for key, value in pairs(data.config) do
    settings[#settings + 1] = { key = key, value = value }
  end
  table.sort(settings, function(a, b)
    return a.key < b.key
  end)
  return {
    title = trx.locale.get(data.name_gs),
    name_gs = data.name_gs,
    settings = settings,
  }
end

-- The name of a preset in the player's language now.
---@param preset common.ui.settings.Preset
---@return string|trx.locale.Text
function M.title(preset)
  if preset.name_gs == nil then
    return preset.title
  end
  return trx.locale.text(preset.name_gs)
end

-- Every preset the game offers, sorted by title. The presets come from the
-- first of the mod's own directory, the directory of the game it sits on, and
-- the configuration directory that has any.
---@return common.ui.settings.Preset[]
function M.load()
  local dir = trx.path.resolve("common_config", "presets")
  local result = {}
  if dir == nil then
    return result
  end
  for _, file in ipairs(dir:list()) do
    if file.name:match("%.json5$") then
      result[#result + 1] = read(file)
    end
  end
  table.sort(result, function(a, b)
    return a.title < b.title
  end)
  return result
end

-- The settings the preset would change. A setting this game does not have, or
-- a value this game does not accept, is passed over, as is one that already
-- holds the value.
---@param preset common.ui.settings.Preset
---@return common.ui.settings.PresetChange[]
function M.changes(preset)
  local result = {}
  for _, entry in ipairs(preset.settings) do
    local target = rows.exists(entry.key)
        and trx.config.to_string(entry.key, { value = entry.value })
      or nil
    if target ~= nil and target ~= trx.config.to_string(entry.key) then
      result[#result + 1] = {
        key = entry.key,
        value = entry.value,
        current = trx.config.to_string(entry.key, { human = true }) or "",
        target = trx.config.to_string(
          entry.key,
          { human = true, value = entry.value }
        ) or "",
      }
    end
  end
  return result
end

-- Applies the preset, and keeps the changes as the player's own. A setting
-- that a script or the game flow holds keeps its value.
---@param preset common.ui.settings.Preset
function M.apply(preset)
  for _, change in ipairs(M.changes(preset)) do
    if not trx.config.is_overridden(change.key) then
      trx.config.set(change.key, change.value)
    end
  end
end

return M
