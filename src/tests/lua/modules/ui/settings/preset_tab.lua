-- The shipped presets tab: the list of presets and the question a picked one
-- asks.

local h = require("harness")
local test = h.test

local Role = trx.input.Role
local accent = require("common.ui.accent")
local preset_tab = require("common.ui.settings.preset_tab")
local rows = require("common.ui.settings.rows")

local function press(role)
  fake.release_all()
  fake.press(role)
  fake.tick()
  fake.release_all()
  fake.tick()
end

local function make(list)
  trx.config.set("ui.text_scale", 1.0)
  local pushed = {}
  local tab = preset_tab.new({
    title = "Presets",
    editing = trx.signal.new(true),
    changed = trx.signal.new(0),
    push = function(settings)
      local layer = trx.ui.layers.push(settings)
      pushed[#pushed + 1] = layer
      return layer
    end,
  })
  tab.presets = list
  tab.scroll:set_count(#list)
  tab:fit(10)
  local layer = trx.ui.layers.push({
    root = tab.widget,
    on_input = function(_, keys)
      tab:control(keys)
    end,
  })
  fake.tick()
  return tab, pushed, layer
end

local WRAP_OFF = {
  title = "Wrap off",
  settings = { { key = "ui.enable_wraparound", value = false } },
}

test("an empty list says so and leaves for the tab bar", function()
  trx.config.set("ui.text_scale", 1.0)
  local tab = preset_tab.new({
    title = "Presets",
    editing = trx.signal.new(true),
    changed = trx.signal.new(0),
    push = trx.ui.layers.push,
  })
  assert(#tab.presets == 0)
  assert(tab:fit(10) == 0)
end)

test("a preset with changes asks before it applies them", function()
  trx.config.set("ui.enable_wraparound", true)
  local tab, pushed, layer = make({ WRAP_OFF })
  tab.scroll:select(1)
  press(Role.MENU_CONFIRM)
  assert(#pushed == 1)
  assert(trx.config.get("ui.enable_wraparound") == true)
  press(Role.MENU_CONFIRM)
  assert(trx.config.get("ui.enable_wraparound") == false)
  assert(not pushed[1].is_open)
  assert(#pushed == 2 and pushed[2].is_open)
  press(Role.MENU_BACK)
  assert(not pushed[2].is_open)
  trx.config.set("ui.enable_wraparound", true)
  layer:close()
end)

test("going back from the question changes nothing", function()
  trx.config.set("ui.enable_wraparound", true)
  local tab, pushed, layer = make({ WRAP_OFF })
  tab.scroll:select(1)
  press(Role.MENU_CONFIRM)
  press(Role.MENU_RIGHT)
  press(Role.MENU_CONFIRM)
  assert(trx.config.get("ui.enable_wraparound") == true)
  assert(not pushed[1].is_open and #pushed == 1)
  layer:close()
end)

test("the question fits a short room by scrolling its text", function()
  trx.config.set("ui.enable_wraparound", true)
  local tab, pushed, layer = make({ WRAP_OFF })
  local safe_area_for = trx.ui.safe_area_for
  trx.ui.safe_area_for = function(width)
    local area = safe_area_for(width)
    area.height = 200
    return area
  end
  tab.scroll:select(1)
  press(Role.MENU_CONFIRM)
  assert(#pushed == 1 and pushed[1].is_open)
  press(Role.MENU_DOWN)
  press(Role.MENU_BACK)
  assert(not pushed[1].is_open)
  assert(trx.config.get("ui.enable_wraparound") == true)
  trx.ui.safe_area_for = safe_area_for
  layer:close()
end)

test("a preset with nothing to change says so", function()
  trx.config.set("ui.enable_wraparound", false)
  local tab, pushed, layer = make({ WRAP_OFF })
  tab.scroll:select(1)
  press(Role.MENU_CONFIRM)
  assert(#pushed == 1)
  press(Role.MENU_CONFIRM)
  assert(not pushed[1].is_open)
  trx.config.set("ui.enable_wraparound", true)
  layer:close()
end)

-- Returns the position and the text of the first text the scene draws that
-- holds `part`.
local function find_text(part)
  for _, op in ipairs(fake.scene()) do
    local x, y, text = op:match("^text x=(%-?%d+) y=(%-?%d+) .- text=(.*)$")
    if text ~= nil and text:find(part, 1, true) ~= nil then
      return tonumber(x), tonumber(y), text
    end
  end
  return nil
end

test("the question shows each change beside its setting, in orange", function()
  fake.set_tr_version(4)
  trx.config.set("ui.enable_wraparound", true)
  local tab, pushed, layer = make({ WRAP_OFF })
  tab.scroll:select(1)
  press(Role.MENU_CONFIRM)
  assert(#pushed == 1 and pushed[1].is_open)
  local name_x, name_y =
    find_text(tostring(rows.title("ui.enable_wraparound")))
  local value_x, value_y, value = find_text("\\{button right}")
  assert(name_x ~= nil and value_x ~= nil, "a change is missing")
  assert(value_y == name_y, "the change stands under its setting")
  assert(value_x > name_x, "the change stands left of its setting")
  local orange = accent.color("", accent.color_of(accent.Role.VALUE))
  assert(value:find(orange, 1, true) ~= nil, "the change is not orange")
  local _, _, title = find_text(
    trx.locale.format("general/config_presets/title_fmt", "Wrap off")
  )
  assert(title:find(orange, 1, true) ~= nil, "the question is not orange")
  press(Role.MENU_BACK)
  layer:close()
end)

test("the question keeps its width as its list scrolls", function()
  -- Short texts around the list, so that the widest setting on screen is what
  -- would set the width of the dialog.
  trx.locale.declare({
    ["general/config_presets/title_fmt"] = "%s?",
    ["general/config_presets/confirm_description"] = "Changes:",
    ["general/config_presets/confirm_restart_note"] = "Note.",
    ["general/config_presets/confirm_apply"] = "Yes",
    ["general/config_presets/confirm_back"] = "No",
  })
  -- The widest settings come first, and leave the screen as the list
  -- scrolls down.
  local keys = {
    "rendering.enable_affine_mapping",
    "rendering.enable_ps1_blending",
    "ui.enable_wraparound",
    "rendering.enable_ps1_fog",
    "audio.enable_ps1_sfx",
    "visuals.enable_gun_glow",
    "visuals.enable_footprints",
    "gameplay.restore_ps1_enemies",
  }
  local preset = { title = "Many", settings = {} }
  local before = {}
  for i, key in ipairs(keys) do
    before[key] = trx.config.get(key)
    trx.config.set(key, true)
    preset.settings[i] = { key = key, value = false }
  end
  local tab, pushed, layer = make({ preset })
  local safe_area_for = trx.ui.safe_area_for
  trx.ui.safe_area_for = function(width)
    local area = safe_area_for(width)
    area.height = 200
    return area
  end
  local function frame_width()
    for _, op in ipairs(fake.scene()) do
      local w = op:match("^text_background .- z=170 w=(%d+)")
      if w ~= nil then
        return tonumber(w)
      end
    end
    return nil
  end
  tab.scroll:select(1)
  press(Role.MENU_CONFIRM)
  assert(#pushed == 1 and pushed[1].is_open)
  local width = frame_width()
  assert(width ~= nil, "no dialog")
  for _ = 1, #keys do
    press(Role.MENU_DOWN)
    assert(frame_width() == width, "the dialog changed its width")
  end
  press(Role.MENU_BACK)
  trx.ui.safe_area_for = safe_area_for
  for key, value in pairs(before) do
    trx.config.set(key, value)
  end
  layer:close()
end)

return h.report()
