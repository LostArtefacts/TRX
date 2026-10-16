-- The shipped config presets: what a preset would change, and applying it.

local h = require("harness")
local test = h.test

local presets = require("common.ui.settings.presets")
require("common.ui.scroll")
require("common.ui.hud.overlay.bars")

local function preset(config)
  local settings = {}
  for key, value in pairs(config) do
    settings[#settings + 1] = { key = key, value = value }
  end
  return { title = "Test", settings = settings }
end

local function find(changes, key)
  for _, change in ipairs(changes) do
    if change.key == key then
      return change
    end
  end
  return nil
end

test("a setting that already holds the value is no change", function()
  trx.config.set("ui.enable_wraparound", true)
  local changes = presets.changes(preset({ ["ui.enable_wraparound"] = true }))
  assert(#changes == 0)
end)

test("a setting this game lacks or cannot take is passed over", function()
  local changes = presets.changes(preset({
    ["visuals.no_such_setting"] = true,
    ["ui.enemy_healthbar_show_mode"] = "not_a_mode",
  }))
  assert(#changes == 0, #changes)
end)

test("a change shows the value held and the value given", function()
  trx.config.set("visuals.fov", 70)
  local changes = presets.changes(preset({ ["visuals.fov"] = 80 }))
  local change = find(changes, "visuals.fov")
  assert(change ~= nil)
  assert(change.current == "70" and change.target == "80")
end)

test("applying a preset sets what it changes", function()
  trx.config.set("ui.enable_wraparound", true)
  trx.config.set("ui.enemy_healthbar_show_mode", "never")
  presets.apply(preset({
    ["ui.enable_wraparound"] = false,
    ["ui.enemy_healthbar_show_mode"] = "always",
  }))
  assert(trx.config.get("ui.enable_wraparound") == false)
  assert(trx.config.get("ui.enemy_healthbar_show_mode") == "always")
  trx.config.set("ui.enable_wraparound", true)
end)

test("a game with no presets directory offers none", function()
  assert(#presets.load() == 0)
end)

return h.report()
