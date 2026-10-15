-- The shipped settings tabs: which settings each tab lists, and where the rows
-- a script declares go among them.

local h = require("harness")
local test = h.test

local tabs = require("common.ui.settings.tabs")

local function index_of(list, key)
  for i, value in ipairs(list) do
    if value == key then
      return i
    end
  end
  return nil
end

test("every tab a declared row can name exists", function()
  for _, name in ipairs({
    "gameplay_general",
    "gameplay_controls",
    "gameplay_mods",
    "gameplay_fixes",
    "graphic_visuals",
    "graphic_ui",
    "graphic_ui_stats",
    "graphic_ui_bars",
    "graphic_rendering",
    "sound_volume",
    "sound_misc",
  }) do
    assert(tabs.exists(name), name)
    assert(#tabs.keys(name) > 0, name)
  end
  assert(not tabs.exists("nowhere"))
end)

test("a declared row with no anchor goes at the end", function()
  local keys = tabs.arrange("sound_volume", { { key = "mod.a" } })
  assert(keys[#keys] == "mod.a")
end)

test("a declared row sits above or below its anchor", function()
  local keys = tabs.arrange("sound_volume", {
    { key = "mod.above", before = "audio.music_volume" },
    { key = "mod.below", after = "audio.music_volume" },
  })
  local music = index_of(keys, "audio.music_volume")
  assert(keys[music - 1] == "mod.above")
  assert(keys[music + 1] == "mod.below")
end)

test("rows declared against one anchor keep their order", function()
  local keys = tabs.arrange("sound_volume", {
    { key = "mod.a", after = "audio.music_volume" },
    { key = "mod.b", after = "audio.music_volume" },
    { key = "mod.c", before = "audio.sound_volume" },
    { key = "mod.d", before = "audio.sound_volume" },
  })
  local music = index_of(keys, "audio.music_volume")
  assert(keys[music + 1] == "mod.a" and keys[music + 2] == "mod.b")
  local sound = index_of(keys, "audio.sound_volume")
  assert(keys[sound - 2] == "mod.c" and keys[sound - 1] == "mod.d")
end)

test("an anchor names a setting by its last segment", function()
  local keys = tabs.arrange("sound_volume", {
    { key = "mod.a", after = "music_volume" },
  })
  assert(keys[index_of(keys, "audio.music_volume") + 1] == "mod.a")
end)

test("an anchor the tab does not list puts the row at the end", function()
  local keys = tabs.arrange("sound_volume", {
    { key = "mod.a", before = "visuals.fov" },
  })
  assert(keys[#keys] == "mod.a")
end)

return h.report()
