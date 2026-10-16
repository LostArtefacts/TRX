-- The shipped settings dialog: its tab bar, its rows, and how the cursor moves
-- between the two.

local h = require("harness")
local test = h.test

local Role = trx.input.Role
local dialog = require("common.ui.settings.dialog")
local editor = require("common.ui.settings.editor")
local tabs = require("common.ui.settings.tabs")

local function press(role)
  fake.release_all()
  fake.press(role)
  fake.tick()
  fake.release_all()
  fake.tick()
end

-- The sound dialog, on a layer that hands it the keys.
local function open(names)
  trx.config.set("ui.text_scale", 1.0)
  local closed = false
  local d = dialog.new({
    title = "Sound",
    tabs = function(self)
      local result = {}
      for i, name in ipairs(names or { "sound_volume", "sound_misc" }) do
        local tab = editor.new({
          title = name,
          keys = tabs.keys(name),
          editing = self.editing,
          width = self.width,
          dialog_width = self.dialog_width,
          changed = self.changed,
          layout = self.layout,
          push = trx.ui.layers.push,
        })
        result[i] = tab
      end
      return result
    end,
  })
  local layer = trx.ui.layers.push({
    root = d.widget,
    place = dialog.PLACE,
    on_input = function(layer, keys)
      if d:control(keys) then
        closed = true
        layer:close()
      end
    end,
  })
  fake.tick()
  return d, layer, function()
    return closed
  end
end

test("left and right switch tabs while the cursor is on the bar", function()
  local d, layer = open()
  press(Role.MENU_RIGHT)
  assert(d.active:get() == 2)
  press(Role.MENU_LEFT)
  assert(d.active:get() == 1)
  layer:close()
  d:close()
end)

test("down enters the rows and up leaves them", function()
  local d, layer = open()
  press(Role.MENU_DOWN)
  assert(d.editing:get())
  assert(d.tabs[1].scroll.selected:get() == 1)
  press(Role.MENU_UP)
  assert(not d.editing:get())
  assert(d.tabs[1].scroll.selected:get() == nil)
  layer:close()
  d:close()
end)

test("left and right change the setting under the cursor", function()
  trx.config.set("audio.master_volume", 50)
  local d, layer = open()
  press(Role.MENU_DOWN)
  assert(d.tabs[1]:selected() == "audio.master_volume")
  press(Role.MENU_RIGHT)
  assert(math.abs(trx.config.get("audio.master_volume") - 0.6) < 0.001)
  assert(d.active:get() == 1)
  layer:close()
  d:close()
end)

test("the tab keys switch tabs from the rows", function()
  local d, layer = open()
  press(Role.MENU_DOWN)
  press(Role.MENU_TAB_RIGHT)
  assert(d.active:get() == 2)
  assert(d.editing:get())
  layer:close()
  d:close()
end)

test("a tab switched to from the rows has its top row selected", function()
  local d, layer = open()
  press(Role.MENU_DOWN)
  press(Role.MENU_DOWN)
  press(Role.STEP_RIGHT)
  assert(d.active:get() == 2)
  assert(d.tabs[2].scroll.selected:get() == 1, d.tabs[2].scroll.selected:get())
  assert(d.tabs[1].scroll.selected:get() == nil)
  layer:close()
  d:close()
end)

test("back closes the dialog from either place", function()
  local d, layer, closed = open()
  press(Role.MENU_DOWN)
  press(Role.MENU_BACK)
  assert(closed())
  assert(not layer.is_open)
  d:close()
end)

test("the dialog keeps a width and shows as many rows as fit", function()
  local d, layer = open()
  assert(d.width:get() > 0)
  local visible = d.tabs[1].scroll.visible:get()
  assert(visible >= 5 and visible <= 16, visible)
  layer:close()
  d:close()
end)

test("the dialog keeps its size whichever tab is active", function()
  local d, layer = open({
    "graphic_visuals",
    "graphic_ui",
    "graphic_ui_stats",
    "graphic_ui_bars",
    "graphic_rendering",
  })
  fake.scene()
  local w, hh = d.widget:measure()
  for i = 2, 5 do
    press(Role.MENU_RIGHT)
    assert(d.active:get() == i)
    fake.scene()
    local w2, h2 = d.widget:measure()
    assert(
      w2 == w and h2 == hh,
      ("tab %d: %gx%g, was %gx%g"):format(i, w2, h2, w, hh)
    )
  end
  layer:close()
  d:close()
end)

-- The text of every label under a widget, in order.
local function texts(widget, out)
  out = out or {}
  if not widget:is_visible() then
    return out
  end
  local text = rawget(widget, "text")
  if text ~= nil then
    out[#out + 1] = type(text) == "table" and text:get() or text
  end
  local child = rawget(widget, "child")
  if child ~= nil then
    texts(child, out)
  end
  for _, other in ipairs(rawget(widget, "children") or {}) do
    texts(other, out)
  end
  return out
end

test("the rows show the settings of the active tab", function()
  local d, layer = open({ "sound_volume", "sound_misc" })
  fake.scene()
  local function has(title)
    for _, text in ipairs(texts(d.widget)) do
      if text:find(title, 1, true) then
        return true
      end
    end
    return false
  end
  local volume = trx.locale.get("settings/audio.master_volume/title")
  local misc = trx.locale.get("settings/audio.enable_music_in_menu/title")
  assert(has(volume) and not has(misc))
  press(Role.MENU_RIGHT)
  fake.scene()
  assert(has(misc) and not has(volume))
  layer:close()
  d:close()
end)

return h.report()
