-- What a settings row shows and how it steps: its title, its value as text,
-- whether the player can change it, and what a press left or right does.
--
-- Usage:
--   local rows = require("common.ui.settings.rows")
--   if rows.can_change("audio.music_volume", 1) then
--     rows.change("audio.music_volume", 1)
--   end

local accent = require("common.ui.accent")
local handlers = require("common.ui.settings.handlers")

local M = {}

-- The bars that a color row dresses, so that the row shows the bar rather
-- than the name of its colors. The PC and the PS1 colors of one bar dress the
-- same bar.
local BAR_COLORS = {
  ["ui.healthbar_color"] = "LARA_HP",
  ["ui.healthbar_poison_color"] = "LARA_HP_POISON",
  ["ui.airbar_color"] = "LARA_AIR",
  ["ui.sprintbar_color"] = "LARA_STAMINA",
  ["ui.exposurebar_color"] = "LARA_EXPOSURE",
  ["ui.enemy_healthbar_color"] = "ENEMY_HP",
  ["ui.enemy_healthbar_color_allies"] = "ALLY_HP",
  ["ui.progressbar_color"] = "PROGRESS",
}

-- What each setting is, as trx.config.describe says, asked once per setting.
-- What a setting is does not change while the game runs, and the game starts
-- the scripts afresh when it switches to another. A setting this game does
-- not have is asked again, because a script can still declare it.
local shapes = {}

local function shape_of(key)
  local shape = shapes[key]
  if shape == nil then
    local ok, found = pcall(trx.config.describe, key)
    if not ok then
      return nil
    end
    shape = found
    shapes[key] = shape
  end
  return shape
end

-- The titles of the settings, for the language they were read in.
local titles = {}
local titles_language = nil

-- What a setting is, as trx.config.describe says. Raises where this game has
-- no such setting.
---@param key string
---@return trx.config.Shape
function M.shape(key)
  local shape = shape_of(key)
  if shape == nil then
    error(("unknown setting: %s"):format(key), 2)
  end
  return shape
end

local function kind(key)
  return M.shape(key).kind
end

local function is_number(shape)
  return shape.kind == "number"
end

-- The game strings that a setting's text is kept under, which the game
-- strings derive from the settings themselves. A title is read once for each
-- language.
function M.title(key)
  local language = trx.signal.config("language"):get()
  if language ~= titles_language then
    titles = {}
    titles_language = language
  end
  local title = titles[key]
  if title == nil then
    title = trx.locale.get(("settings/%s/title"):format(key))
    titles[key] = title
  end
  return title
end

function M.description(key)
  return trx.locale.get(("settings/%s/description"):format(key))
end

-- Whether the settings file names a setting this game has.
---@param key string
---@return boolean
function M.exists(key)
  return shape_of(key) ~= nil
end

-- Whether a script or the game flow holds the setting, so that it is not the
-- player's to change.
function M.held(key)
  return trx.config.is_overridden(key)
end

-- The bar a color row dresses, or nil for a row that is not a bar's color.
---@param key string
---@return trx.ui.BarType?
function M.bar_type(key)
  local name = BAR_COLORS[key] or BAR_COLORS[key:gsub("_ps1$", "")]
  return name ~= nil and trx.ui.BarType[name] or nil
end

-- Whether the row is a color that the color editor changes.
function M.is_color(key)
  return kind(key) == "color"
end

-- The values an enum row offers, in the order the row steps through them,
-- each with whether it can be picked now.
---@param key string
---@return { value: string, label: string, enabled: boolean }[]
function M.choices(key)
  local labels = trx.config.labels(key)
  local handler = handlers.get(key)
  local result = {}
  if handler.order ~= nil then
    local by_value = {}
    for _, entry in ipairs(labels) do
      by_value[entry.value] = entry
    end
    for _, value in ipairs(handler.order) do
      if by_value[value] ~= nil then
        result[#result + 1] = by_value[value]
      end
    end
  else
    result = labels
  end
  if handler.offered ~= nil then
    for i, entry in ipairs(result) do
      result[i] = {
        value = entry.value,
        label = entry.label,
        enabled = entry.enabled and handler.offered(entry.value),
      }
    end
  end
  return result
end

local function is_enum(key)
  local k = kind(key)
  return k == "enum" or k == "dynamic_enum"
end

-- The next value that can be picked, one step to a side of the current one.
local function next_choice(key, dir)
  local list = M.choices(key)
  local current = trx.config.get(key)
  local from = 0
  for i, entry in ipairs(list) do
    if entry.value == current then
      from = i
    end
  end
  if from == 0 then
    dir = 1
  end
  local step = dir < 0 and -1 or 1
  local i = from + step
  while list[i] ~= nil do
    if list[i].enabled then
      return list[i].value
    end
    i = i + step
  end
  return nil
end

-- Whether the setting is left out of its tab.
---@param key string
---@return boolean
function M.hidden(key)
  local handler = handlers.get(key)
  if handler.visible ~= nil and not handler.visible() then
    return true
  end
  if trx.config.is_hidden(key) then
    return true
  end
  if kind(key) == "enum" then
    for _, entry in ipairs(M.choices(key)) do
      if entry.enabled then
        return false
      end
    end
    return true
  end
  return false
end

-- Whether the row can be changed, rather than greyed out.
function M.available(key)
  local handler = handlers.get(key)
  return not M.held(key)
    and (handler.available == nil or handler.available() == true)
end

local function dim(text, on)
  if on then
    return text
  end
  return accent.text(text, accent.Role.DIM)
end

-- What a row shows of each setting, kept as one signal per setting that every
-- row and every dialog showing the setting shares.
local states = {}

-- What a row shows of a setting: the title, starred where something holds the
-- setting, and the value, both greyed out where the player cannot change it,
-- and the color of a color setting. It is worked out once each time a setting
-- changes, and only while something reads it.
---@param key string
---@return trx.signal.Signal
function M.state(key)
  local state = states[key]
  if state == nil then
    state = trx.config.signals.changed:map(function()
      local held = M.held(key)
      local handler = handlers.get(key)
      local available = not held
        and (handler.available == nil or handler.available() == true)
      return {
        title = dim(M.title(key) .. (held and "*" or ""), available),
        value = dim(M.value_text(key), available and M.on_offer(key)),
        color = M.is_color(key) and trx.config.get(key) or nil,
      }
    end)
    states[key] = state
  end
  return state
end

-- Whether the value the setting holds can be picked now. A dynamic enum can
-- hold a value it no longer offers, such as a language whose files went.
function M.on_offer(key)
  if kind(key) ~= "dynamic_enum" then
    return true
  end
  local current = trx.config.get(key)
  for _, entry in ipairs(trx.config.labels(key)) do
    if entry.value == current then
      return entry.enabled
    end
  end
  return false
end

-- A color as the settings show it, such as `#1A2B3C`.
---@param color trx.math.Color
---@return string
function M.hex(color)
  return ("#%02X%02X%02X"):format(color.r, color.g, color.b)
end

-- The value as the row prints it.
---@param key string
---@param value? any A value to print in place of the setting's own.
---@return string
function M.value_text(key, value)
  local handler = handlers.get(key)
  local own = value == nil
  if own then
    value = trx.config.get(key)
  end
  local formatted = handler.key_format ~= nil and handler.key_format(value)
  if formatted then
    return formatted
  elseif M.is_color(key) then
    return M.hex(value)
  elseif own then
    return trx.config.to_string(key, { human = true }) or ""
  end
  -- A value given here is the one the setting stores, and a percentage is
  -- read the way it is set, from 0 to 100.
  if M.shape(key).percent then
    value = value * 100
  end
  return trx.config.to_string(key, { human = true, value = value }) or ""
end

-- The texts the value can take, for working out how wide its column is.
---@param key string
---@return string[]
function M.value_texts(key)
  local handler = handlers.get(key)
  if handler.key_format ~= nil or M.is_color(key) then
    return { M.value_text(key) }
  end
  local shape = M.shape(key)
  if shape.kind == "boolean" then
    return { M.value_text(key, false), M.value_text(key, true) }
  elseif shape.kind == "integer" or is_number(shape) then
    if shape.min == nil or shape.max == nil then
      return { M.value_text(key) }
    end
    return { M.value_text(key, shape.min), M.value_text(key, shape.max) }
  elseif shape.kind == "string" then
    return { tostring(trx.config.get(key)) }
  elseif is_enum(key) then
    local current = trx.config.get(key)
    local texts = {}
    for _, entry in ipairs(M.choices(key)) do
      if
        shape.kind == "dynamic_enum"
        or entry.enabled
        or entry.value == current
      then
        texts[#texts + 1] = entry.label
      end
    end
    return texts
  end
  return {}
end

-- How far a press moves the setting, in its own steps: whole numbers for an
-- integer, hundredths for a number.
local function delta(key, dir, fine)
  local handler = handlers.get(key)
  local step = (fine and handler.slow or handler.fast) or 1
  if step == 0 then
    step = 1
  end
  return step * dir
end

local function hundredths(value)
  return math.floor(value * 100 + 0.5)
end

-- Whether a press to a side would change the setting.
---@param key string
---@param dir integer `-1` for the left, `1` for the right.
---@return boolean
function M.can_change(key, dir)
  if M.held(key) then
    return false
  end
  local handler = handlers.get(key)
  if handler.can_change ~= nil then
    return handler.can_change(key, dir) == true
  end
  local shape = M.shape(key)
  local value = trx.config.get(key)
  if shape.kind == "boolean" then
    return true
  elseif shape.kind == "integer" then
    if dir < 0 then
      return shape.min == nil or value > shape.min
    end
    return shape.max == nil or value < shape.max
  elseif is_number(shape) then
    local target = (hundredths(value) + dir) / 100
    return (shape.min == nil or target >= shape.min)
      and (shape.max == nil or target <= shape.max)
  elseif is_enum(key) then
    return next_choice(key, dir) ~= nil
  end
  return false
end

local function clamp(shape, value)
  if shape.min ~= nil then
    value = math.max(value, shape.min)
  end
  if shape.max ~= nil then
    value = math.min(value, shape.max)
  end
  return value
end

-- Changes the setting one press to a side, as the player does. Returns whether
-- it changed.
---@param key string
---@param dir integer `-1` for the left, `1` for the right.
---@param fine? boolean Whether fine adjustment is held.
---@return boolean
function M.change(key, dir, fine)
  if not M.can_change(key, dir) then
    return false
  end
  local handler = handlers.get(key)
  if handler.takes_over ~= nil and handler.takes_over(key, dir) then
    -- The script moved the setting, or chose to leave it.
  elseif handler.change ~= nil then
    if not handler.change(key, dir) then
      return false
    end
  else
    local shape = M.shape(key)
    local value = trx.config.get(key)
    if shape.kind == "boolean" then
      trx.config.set(key, not value)
    elseif shape.kind == "integer" then
      trx.config.set(key, clamp(shape, value + delta(key, dir, fine)))
    elseif is_number(shape) then
      local target = (hundredths(value) + delta(key, dir, fine)) / 100
      target = clamp(shape, target)
      -- A percentage is set the way it is shown, from 0 to 100.
      trx.config.set(key, shape.percent and target * 100 or target)
    elseif is_enum(key) then
      trx.config.set(key, next_choice(key, dir))
    end
  end
  if handler.after_change ~= nil then
    handler.after_change()
  end
  return true
end

-- Puts the setting back to its default. Returns whether it changed.
---@param key string
---@return boolean
function M.restore(key)
  if M.held(key) or trx.config.is_default(key) then
    return false
  end
  trx.config.reset(key)
  return true
end

return M
