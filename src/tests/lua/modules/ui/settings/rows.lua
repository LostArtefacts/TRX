-- The shipped settings rows: what a row prints, and how a press steps it.

local h = require("harness")
local test = h.test

local rows = require("common.ui.settings.rows")

local function with(key, value, fn)
  local old = trx.config.get(key)
  trx.config.set(key, value)
  local ok, err = pcall(fn)
  trx.config.set(key, old)
  assert(ok, err)
end

test("a toggle flips either way", function()
  with("ui.enable_wraparound", true, function()
    assert(rows.can_change("ui.enable_wraparound", -1))
    assert(rows.change("ui.enable_wraparound", 1))
    assert(trx.config.get("ui.enable_wraparound") == false)
  end)
end)

test("a number stops at its bounds", function()
  local shape = trx.config.describe("visuals.fov")
  with("visuals.fov", shape.max, function()
    assert(not rows.can_change("visuals.fov", 1))
    assert(rows.can_change("visuals.fov", -1))
    assert(rows.change("visuals.fov", -1))
    assert(trx.config.get("visuals.fov") == shape.max - 5)
    assert(rows.change("visuals.fov", -1, true))
    assert(trx.config.get("visuals.fov") == shape.max - 6)
  end)
end)

test("a percentage steps in hundredths of what it stores", function()
  with("audio.music_volume", 50, function()
    assert(rows.change("audio.music_volume", 1))
    local value = trx.config.get("audio.music_volume")
    assert(math.abs(value - 0.6) < 0.001, value)
    assert(rows.change("audio.music_volume", -1, true))
    value = trx.config.get("audio.music_volume")
    assert(math.abs(value - 0.59) < 0.001, value)
  end)
end)

test("a percentage prints from 0 to 100", function()
  with("audio.music_volume", 60, function()
    assert(rows.value_text("audio.music_volume") == "60%")
    local texts = rows.value_texts("audio.music_volume")
    assert(texts[1] == "0%" and texts[2] == "100%", table.concat(texts, ","))
  end)
end)

test("an enum steps in the order its handler gives", function()
  with("ui.enemy_healthbar_show_mode", "never", function()
    assert(not rows.can_change("ui.enemy_healthbar_show_mode", -1))
    assert(rows.change("ui.enemy_healthbar_show_mode", 1))
    assert(trx.config.get("ui.enemy_healthbar_show_mode") == "boss_only")
  end)
end)

test("an enum value that is not offered is stepped over", function()
  -- No level is loaded, so the patterned backgrounds are not on offer.
  local key = "ui.inventory_background_style"
  with(key, "black", function()
    assert(rows.change(key, 1))
    assert(trx.config.get(key) == "monochrome", trx.config.get(key))
  end)
end)

test("a row greys out with the setting it follows", function()
  with("ui.show_bars", false, function()
    assert(not rows.available("ui.bar_scale"))
  end)
  with("ui.show_bars", true, function()
    assert(rows.available("ui.bar_scale"))
  end)
end)

test("a color row prints its value as hex", function()
  local text = rows.value_text("visuals.water_color")
  assert(text:match("^#%x%x%x%x%x%x$"), text)
  assert(not rows.can_change("visuals.water_color", 1))
end)

test("a bar color row names its bar", function()
  assert(rows.bar_type("ui.healthbar_color") == trx.ui.BarType.LARA_HP)
  assert(rows.bar_type("ui.airbar_color_ps1") == trx.ui.BarType.LARA_AIR)
  assert(rows.bar_type("visuals.fov") == nil)
end)

test("restoring the default changes a setting only once", function()
  with("visuals.fov", 70, function()
    if trx.config.is_default("visuals.fov") then
      return
    end
    assert(rows.restore("visuals.fov"))
    assert(not rows.restore("visuals.fov"))
  end)
end)

test("a setting this game lacks does not exist", function()
  assert(rows.exists("visuals.fov"))
  assert(not rows.exists("visuals.no_such_setting"))
end)

return h.report()
