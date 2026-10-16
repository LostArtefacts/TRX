-- The looks and colors of the game's bars and the colors of its menus, as
-- cfg/ui.json5 states them, and the settings that choose between them.
--
-- The file is read once the game paths are known, just before the startup
-- scripts run. The settings are declared then, because the looks and colors
-- that the file states are the values that the settings take.
--
-- The C bridges are captured as the module loads, and some are absent where
-- a test leaves them out.
local raw_events = trxc.events
local raw_game = trxc.game
local raw_json = trxc.json
local raw_path = trxc.path
local h = require("trx.internal.helpers")
require("trx.config")
require("trx.locale")

local M = {}

local STEPS = 5

-- The value that makes a bar follow the look the rest of the bars are drawn
-- with, rather than naming a look of its own.
local INHERIT = "same"
local INHERIT_LABEL = "dynamic/enums/bar_look/same"
trx.locale.declare({ ["dynamic/enums/bar_look/same"] = "Automatic" })

-- The default look of the bars, and of the progress bar, in each game.
local BAR_LOOK_DEFAULTS = { "tr1_pc", "tr2_ps1", "tr3_ps1", "tr3_ps1" }
local PROGRESS_LOOK_DEFAULTS = { INHERIT, INHERIT, INHERIT, "loading" }

-- Each bar by its `trx.ui.BarType` value: the name `bar_colors` knows it by,
-- and the settings that choose its PC and PS1 colors. The progress bar picks
-- a look of its own and a default color for each game.
local BARS = {
  [0] = {
    name = "lara_hp",
    pc = { key = "ui.healthbar_color", default = "red" },
    ps1 = { key = "ui.healthbar_color_ps1", default = "red-green" },
  },
  [1] = {
    name = "lara_hp_poison",
    pc = { key = "ui.healthbar_poison_color", default = "yellow" },
    ps1 = { key = "ui.healthbar_poison_color_ps1", default = "yellow" },
  },
  [2] = {
    name = "lara_air",
    pc = { key = "ui.airbar_color", default = "blue" },
    ps1 = { key = "ui.airbar_color_ps1", default = "teal-green" },
  },
  [3] = {
    name = "lara_stamina",
    pc = { key = "ui.sprintbar_color", default = "green" },
    ps1 = { key = "ui.sprintbar_color_ps1", default = "red-yellow" },
  },
  [4] = {
    name = "lara_exposure",
    pc = { key = "ui.exposurebar_color", default = "cyan" },
    ps1 = { key = "ui.exposurebar_color_ps1", default = "dark-blue-red" },
  },
  [5] = {
    name = "enemy_hp",
    pc = { key = "ui.enemy_healthbar_color", default = "grey" },
    ps1 = { key = "ui.enemy_healthbar_color_ps1", default = "orange-red" },
  },
  [6] = {
    name = "ally_hp",
    pc = { key = "ui.enemy_healthbar_color_allies", default = "teal" },
    ps1 = {
      key = "ui.enemy_healthbar_color_allies_ps1",
      default = "yellow-green",
    },
  },
  [7] = {
    name = "progress",
    look = { key = "ui.progressbar_look" },
    pc = {
      key = "ui.progressbar_color",
      default = { "maroon", "purple", "purple", "purple-gradient" },
    },
    ps1 = {
      key = "ui.progressbar_color_ps1",
      default = { "maroon", "purple-teal", "purple-teal", "purple-gradient" },
    },
  },
}
local BAR_LOOK = { key = "ui.bar_look" }

local TR_KEYS = { "tr1", "tr2", "tr3", "tr4" }

local MENU_PS1_FIELDS = {
  "background_edge",
  "background_center",
  "background_heavy_edge",
  "background_heavy_center",
  "heading_edge",
  "heading_center",
  "requested_edge",
  "requested_center",
  "requested_outline_ch",
  "requested_outline_cv",
  "requested_outline_edge",
  "outline_tl",
  "outline_tr",
  "outline_bl",
  "outline_br",
  "heading_outline",
}

-- The looks in the order the file states them, and each by name.
local looks = {}
local looks_by_name = {}
-- The colors each bar offers where the file narrows them, by bar name.
---@type table<string, string[]>
local filters = {}
-- The menu colors of each game, by its number.
local menus = {}
-- The theme each bar is drawn with, worked out on first use and dropped when
-- a setting that chooses it changes.
local resolved = {}

local function fail(value, message, ...)
  local where = raw_json ~= nil and raw_json.where(value) or nil
  error(("%s: " .. message):format(where or "ui.json5", ...), 0)
end

-- Returns the keys of an object in the order the file states them. A table
-- that was not read from a file gives its keys in name order.
local function keys_of(object)
  local keys = raw_json ~= nil and raw_json.keys(object) or nil
  if keys == nil then
    keys = {}
    for key in pairs(object) do
      keys[#keys + 1] = key
    end
    table.sort(keys)
  end
  return keys
end

local function game_version()
  return raw_game ~= nil and raw_game.get_version() or nil
end

local function color(text, owner)
  if type(text) ~= "string" then
    fail(owner, "a color must be hex text")
  end
  local hex = text:gsub("^#", "")
  if not hex:match("^%x+$") or (#hex ~= 6 and #hex ~= 8) then
    fail(owner, "%s is not a color", text)
  end
  return {
    r = tonumber(hex:sub(1, 2), 16),
    g = tonumber(hex:sub(3, 4), 16),
    b = tonumber(hex:sub(5, 6), 16),
    a = #hex == 8 and tonumber(hex:sub(7, 8), 16) or 255,
  }
end

local function ramp(list, owner)
  if type(list) ~= "table" or #list ~= STEPS then
    fail(owner, "invalid color array (expected %d entries)", STEPS)
  end
  local out = {}
  for i = 1, STEPS do
    out[i] = color(list[i], list)
  end
  return out
end

local function read_look(name, data)
  if type(data.name_gs) ~= "string" then
    fail(data, "look %s has no name_gs", name)
  end
  local kind = data.style
  if kind ~= "pc" and kind ~= "ps1" then
    fail(data, "invalid 'style' value '%s'", tostring(kind))
  end
  local colors = data.colors
  if type(colors) ~= "table" or next(colors) == nil then
    fail(data, "'colors' of look %s must be an object that is not empty", name)
  end

  local look = {
    name = name,
    label = data.name_gs,
    kind = kind,
    colors = {},
    order = {},
  }
  local base = { kind = kind, basic_scale = data.scale or 1 }
  if kind == "pc" then
    base.border_light = color(data.border_light, data)
    base.border_dark = color(data.border_dark, data)
  else
    base.border_tl = color(data.border_tl, data)
    base.border_tr = color(data.border_tr, data)
    base.border_bl = color(data.border_bl, data)
    base.border_br = color(data.border_br, data)
  end

  for _, color_name in ipairs(keys_of(colors)) do
    local theme = {}
    for field, value in pairs(base) do
      theme[field] = value
    end
    local entry = colors[color_name]
    if kind == "pc" then
      theme.ramp = ramp(entry, colors)
    else
      if type(entry) ~= "table" or #entry ~= 2 then
        fail(
          colors,
          "invalid '%s' color definition (expected 2 arrays)",
          color_name
        )
      end
      theme.ramp_left = ramp(entry[1], entry)
      theme.ramp_right = ramp(entry[2], entry)
    end
    look.colors[color_name] = theme
    look.order[#look.order + 1] = color_name
  end
  return look
end

local function read_filters(data)
  local bar_names = {}
  for _, bar in pairs(BARS) do
    bar_names[bar.name] = true
  end
  local out = {}
  for name, list in pairs(data or {}) do
    if not bar_names[name] then
      fail(data, "unknown bar '%s'", name)
    end
    if type(list) ~= "table" or #list == 0 then
      fail(data, "'%s' cannot be empty", name)
    end
    out[name] = list
  end
  return out
end

local function read_menus(data)
  local out = {}
  for version, key in ipairs(TR_KEYS) do
    local game = data ~= nil and data[key] or nil
    if type(game) ~= "table" then
      fail(data, "the menu colors of %s are missing", key)
    end
    local pc, ps1 = game.pc, game.ps1
    out[version] = {
      pc = {
        background = {
          color(pc.background[1], pc),
          color(pc.background[2], pc),
        },
        background_heavy = {
          color(pc.background_heavy[1], pc),
          color(pc.background_heavy[2], pc),
        },
        outline_light = color(pc.outline_light, pc),
        outline_dark = color(pc.outline_dark, pc),
      },
      ps1 = {},
    }
    for _, field in ipairs(MENU_PS1_FIELDS) do
      out[version].ps1[field] = color(ps1[field], ps1)
    end
  end
  return out
end

local function has_color(kind, name)
  for _, look in ipairs(looks) do
    if look.kind == kind and look.colors[name] ~= nil then
      return true
    end
  end
  return false
end

-- The colors a bar offers in looks of one kind: those the file narrows it to
-- that such a look states, or else every color those looks state, each once.
local function offered_colors(bar, kind)
  local out = {}
  local filter = filters[bar.name]
  if filter ~= nil then
    for _, name in ipairs(filter) do
      if has_color(kind, name) then
        out[#out + 1] = { value = name }
      end
    end
    return out
  end
  local seen = {}
  for _, look in ipairs(looks) do
    if look.kind == kind then
      for _, name in ipairs(look.order) do
        if not seen[name] then
          seen[name] = true
          out[#out + 1] = { value = name }
        end
      end
    end
  end
  return out
end

-- The default the setting states for this game, or the first value it offers
-- where the file leaves that one out.
local function pick_default(default, values)
  if type(default) == "table" then
    default = default[game_version()] or default[1]
  end
  for _, entry in ipairs(values) do
    if entry.value == default then
      return default
    end
  end
  return values[1] ~= nil and values[1].value or default
end

local function forget()
  resolved = {}
end

-- Reads a setting, or nil where no option has that key.
local function read(key)
  local ok, value = pcall(trx.config.get, key)
  if ok then
    return value
  end
  return nil
end

local function declare(setting, values, default)
  local key = setting.key
  if pcall(trx.config.describe, key) then
    return
  end
  trx.config.declare({
    key = key,
    kind = "dynamic_enum",
    values = values,
    default = pick_default(default, values),
  })
  trx.config.on_change(key, forget)
end

local function declare_settings()
  if not pcall(trx.config.describe, "ui.enable_smooth_bars") then
    trx.config.declare({
      key = "ui.enable_smooth_bars",
      kind = "boolean",
      default = false,
    })
  end
  local look_values = {}
  for _, look in ipairs(looks) do
    look_values[#look_values + 1] = { value = look.name, label = look.label }
  end
  declare(BAR_LOOK, look_values, BAR_LOOK_DEFAULTS)

  for type = 0, #BARS do
    local bar = BARS[type]
    if bar.look ~= nil then
      local values = { { value = INHERIT, label = INHERIT_LABEL } }
      for _, entry in ipairs(look_values) do
        values[#values + 1] = entry
      end
      declare(bar.look, values, PROGRESS_LOOK_DEFAULTS)
    end
    declare(bar.pc, offered_colors(bar, "pc"), bar.pc.default)
    declare(bar.ps1, offered_colors(bar, "ps1"), bar.ps1.default)
  end
end

-- Takes the looks, colors and menu colors from the decoded contents of a file
-- shaped as cfg/ui.json5 is, and declares the settings that choose between
-- them.
function M.load(data)
  if type(data) ~= "table" or type(data.bars) ~= "table" then
    fail(data, "the file states no bar looks")
  end
  local new_looks = {}
  local new_by_name = {}
  for _, name in ipairs(keys_of(data.bars)) do
    local look = read_look(name, data.bars[name])
    new_looks[#new_looks + 1] = look
    new_by_name[name] = look
  end
  if #new_looks == 0 then
    fail(data.bars, "the file states no bar looks")
  end
  local new_filters = read_filters(data.bar_colors)
  local new_menus = read_menus(data.ui)

  looks, looks_by_name = new_looks, new_by_name
  filters, menus = new_filters, new_menus
  forget()
  declare_settings()
end

-- Returns the look a bar is drawn with. A bar that names no look of its own
-- takes the one the rest of the bars are drawn with, and an unknown look
-- falls back to the first one the file states.
local function look_of(bar)
  if #looks == 0 then
    return nil
  end
  if bar.look ~= nil then
    local name = read(bar.look.key)
    if name ~= nil and name ~= INHERIT and looks_by_name[name] ~= nil then
      return looks_by_name[name]
    end
  end
  return looks_by_name[read(BAR_LOOK.key)] or looks[1]
end

-- Returns whether a bar of the given `trx.ui.BarType` takes a PS1 look.
function M.is_ps1(type)
  local bar = BARS[type]
  if bar == nil then
    error("unknown bar type", 2)
  end
  local look = look_of(bar)
  return look ~= nil and look.kind == "ps1"
end

-- Returns the theme a bar of the given `trx.ui.BarType` is drawn with, or
-- nil before the file is read. The theme is shared: it is the same table for
-- as long as the settings that choose it stay as they are.
function M.bar(type)
  local bar = BARS[type]
  if bar == nil then
    error("unknown bar type", 2)
  end
  local theme = resolved[type]
  if theme ~= nil then
    return theme
  end
  local look = look_of(bar)
  if look == nil then
    return nil
  end
  local name = read(bar[look.kind].key)
  theme = look.colors[name] or look.colors[look.order[1]]
  resolved[type] = theme
  return theme
end

-- Returns the menu colors of this game in its PC and PS1 styles, or nil
-- before the file is read.
function M.menu()
  return menus[game_version()]
end

raw_events.attach(h.constants("LUA_EVENT_TYPE").STARTUP, function()
  local path = raw_path.resolve("common_config", "ui.json5")
  local text = path ~= nil and raw_path.read_text(path) or nil
  if path == nil or text == nil then
    error("ui.json5 could not be read", 0)
  end
  M.load(raw_json.decode_from(text, path))
end)

return M
