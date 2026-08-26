-- The config API as a script actually sees it.
--
-- The registry, the override stack and the settings rows underneath are the
-- real ones, so what these assert is what matters: a script can add a setting
-- of its own, hear about the ones that move, and hold one away from the
-- player's value and give it back without ever writing to their settings file.

local h = require("harness")
local test, raises = h.test, h.raises

test("a value reads back as the type the option is declared with", function()
  assert(
    trx.config.get("audio.enable_music") == true,
    "a bool option must read as a bool"
  )
  assert(
    trx.config.get("visuals.fov") == 65,
    "an int option must read as a number"
  )
  assert(
    trx.config.get("visuals.brightness") == 1.5,
    "a double option must read as a number"
  )
  assert(
    trx.config.get("visuals.water_color") == trx.math.color("ff0000"),
    "a color reads as a color"
  )
  assert(trx.config.get("visuals.water_color").hex == "ff0000")
end)

test("a value is written as the type it is, not as a string", function()
  trx.config.set("audio.enable_music", false)
  assert(
    trx.config.get("audio.enable_music") == false,
    "a boolean did not go through"
  )

  trx.config.set("visuals.fov", 90)
  assert(trx.config.get("visuals.fov") == 90, "a number did not go through")
end)

test("a color is written as hex text or as a color", function()
  trx.config.set("visuals.water_color", "0080ff")
  assert(trx.config.get("visuals.water_color").hex == "0080ff")

  trx.config.set("visuals.water_color", trx.math.color(0, 255, 192))
  assert(trx.config.get("visuals.water_color").hex == "00ffc0")
end)

test("an unknown option raises rather than reading nil", function()
  raises(function()
    trx.config.get("visuals.nonsense")
  end)
  raises(function()
    trx.config.set("visuals.nonsense", 1)
  end)
  raises(function()
    trx.config.reset("visuals.nonsense")
  end)
end)

test("reset brings the default back and keeps it", function()
  trx.config.set("visuals.fov", 90)
  local writes = fake.calls().config_write.count
  assert(trx.config.reset("visuals.fov") == true)
  assert(trx.config.get("visuals.fov") == 65, "the default did not come back")
  assert(fake.calls().config_write.count == writes + 1, "reset must persist")
end)

test("a setting the game flow enforces cannot be reset", function()
  fake.set_enforced(true)
  assert(trx.config.reset("visuals.fov") == false)
end)

test("force writes through an enforced setting", function()
  fake.set_enforced(true)
  raises(function()
    trx.config.set("visuals.fov", 90)
  end)
  trx.config.set("visuals.fov", 90, true)
  assert(trx.config.get("visuals.fov") == 90, "the forced write did not take")

  assert(trx.config.reset("visuals.fov", true) == true)
  assert(trx.config.get("visuals.fov") == 65, "the forced reset did not take")
end)

test("a value that will not parse raises", function()
  raises(function()
    trx.config.set("visuals.fov", "wide")
  end)
  assert(trx.config.get("visuals.fov") == 65, "the option must be left alone")
end)

test("set writes the player's settings; override does not", function()
  trx.config.set("visuals.fov", 90)
  assert(fake.calls().config_write.count == 1, "set must persist")

  trx.config.override("visuals.fov", 100)
  assert(
    fake.calls().config_write.count == 1,
    "an override must never reach the settings file"
  )
end)

test(
  "an override holds the setting, and restore gives the player theirs back",
  function()
    trx.config.override("visuals.fov", 90)
    assert(trx.config.get("visuals.fov") == 90, "the override did not take")
    assert(trx.config.is_overridden("visuals.fov") == true)

    assert(trx.config.restore("visuals.fov") == true)
    -- The player chose 65. That is what comes back - not a default, not 90.
    assert(
      trx.config.get("visuals.fov") == 65,
      "the player's value did not come back"
    )
    assert(trx.config.is_overridden("visuals.fov") == false)
  end
)

test("overrides stack, and each restore lifts one", function()
  trx.config.override("visuals.fov", 90)
  trx.config.override("visuals.fov", 120)
  assert(trx.config.get("visuals.fov") == 120)

  trx.config.restore("visuals.fov")
  assert(
    trx.config.get("visuals.fov") == 90,
    "the one underneath is the other override"
  )

  trx.config.restore("visuals.fov")
  assert(
    trx.config.get("visuals.fov") == 65,
    "and under that, the player's own"
  )
end)

test("a color option is given back by value", function()
  trx.config.override("visuals.water_color", "0080ff")
  assert(trx.config.get("visuals.water_color").hex == "0080ff")

  trx.config.restore("visuals.water_color")
  assert(
    trx.config.get("visuals.water_color").hex == "ff0000",
    "the player's color did not come back"
  )
end)

test("restoring what was never overridden reports false", function()
  assert(trx.config.restore("visuals.fov") == false)
  assert(trx.config.get("visuals.fov") == 65)
end)

test("a setting the game flow enforces cannot be overridden", function()
  fake.set_enforced(true)
  raises(function()
    trx.config.override("visuals.fov", 90)
  end)
  assert(trx.config.get("visuals.fov") == 65)
  -- what the level asked for is still what holds the setting; the script's
  -- override did not go over the top of it
  assert(trx.config.is_overridden("visuals.fov") == true)
end)

test("an enum option reads and writes by value name", function()
  assert(trx.config.get("visuals.shadow_type") == "circle")
  trx.config.set("visuals.shadow_type", "sprite")
  assert(trx.config.get("visuals.shadow_type") == "sprite")
  raises(function()
    trx.config.set("visuals.shadow_type", "nonsense")
  end)
end)

test("an enum value is taken in either spelling", function()
  trx.config.set("visuals.shadow_type", "extra-dark")
  assert(trx.config.get("visuals.shadow_type") == "extra_dark")
end)

test("describe tells the shape of a setting", function()
  assert(trx.config.describe("audio.enable_music").kind == "boolean")
  assert(trx.config.describe("visuals.fov").kind == "integer")
  assert(trx.config.describe("visuals.water_color").kind == "color")

  local brightness = trx.config.describe("visuals.brightness")
  assert(brightness.kind == "number")
  assert(brightness.percent == false)

  local shadow = trx.config.describe("visuals.shadow_type")
  assert(shadow.kind == "enum")
  local values = table.concat(shadow.values, ",")
  assert(values:find("circle"), values)
  assert(values:find("sprite"), values)
  assert(values:find("extra_dark"), values)

  raises(function()
    trx.config.describe("visuals.nonsense")
  end)
end)

test("format_value spells a value the way the console prints it", function()
  assert(trx.config.format_value("audio.enable_music") == "1")
  trx.config.set("audio.enable_music", false)
  assert(trx.config.format_value("audio.enable_music") == "0")

  assert(trx.config.format_value("visuals.fov") == "65")
  assert(trx.config.format_value("visuals.brightness") == "1.50")
  assert(trx.config.format_value("audio.master_volume") == "100%")
  assert(trx.config.format_value("visuals.water_color") == "ff0000")

  trx.config.set("visuals.shadow_type", "extra_dark")
  assert(trx.config.format_value("visuals.shadow_type") == "extra-dark")
end)

test("accepted_values says what a setting takes", function()
  -- The type markers are the text the module declares for them. A percentage
  -- and a plain integer read alike, because that is what they say.
  assert(trx.config.accepted_values("audio.enable_music") == "on, off")
  assert(trx.config.accepted_values("visuals.fov") == "[integer]")
  assert(trx.config.accepted_values("visuals.brightness") == "[decimal]")
  assert(trx.config.accepted_values("audio.master_volume") == "[integer]")
  assert(trx.config.accepted_values("visuals.water_color") == nil)

  local values = trx.config.accepted_values("visuals.shadow_type")
  assert(values:find("circle", 1, true), values)
  assert(values:find("extra-dark", 1, true), values)
  assert(not values:find("extra_dark", 1, true), values)
end)

test("list gives every setting, typed", function()
  local all = trx.config.list()
  assert(all["audio.enable_music"] == true, "a bool must be listed as a bool")
  assert(all["visuals.fov"] == 65)
  assert(all["visuals.water_color"].hex == "ff0000")
end)

test("describe hands back the shape a declaration takes", function()
  local shape = trx.config.describe("visuals.fov")
  assert(shape.key == "visuals.fov")
  assert(shape.kind == "integer")
  assert(shape.default == 65, "the default must come back with the shape")

  local color = trx.config.describe("visuals.water_color")
  assert(color.default.hex == "ff0000")
end)

test("a game declares a setting of its own", function()
  trx.config.declare({
    key = "mod.scanlines",
    kind = "boolean",
    default = true,
  })
  assert(trx.config.get("mod.scanlines") == true, "the default did not take")

  trx.config.set("mod.scanlines", false)
  assert(trx.config.get("mod.scanlines") == false, "the write did not take")
end)

test("a declared setting takes a row on a tab", function()
  trx.config.declare({
    key = "mod.grain",
    kind = "integer",
    default = 2,
    min = 0,
    max = 4,
    ui = { tab = "graphic_visuals", after = "visuals.fov" },
  })
  assert(trx.config.get("mod.grain") == 2)
end)

test("a row does what its handlers say", function()
  local available = false
  trx.config.declare({
    key = "mod.bloom",
    kind = "integer",
    default = 50,
    min = 0,
    max = 100,
    ui = {
      tab = "graphic_visuals",
      delta_fast = 5,
      delta_slow = 1,
      format_value = function(value)
        return value .. "%"
      end,
      is_available = function()
        return available
      end,
    },
  })
  assert(trx.config.get("mod.bloom") == 50)
  available = true
end)

test("a tab no dialog shows raises", function()
  raises(function()
    trx.config.declare({
      key = "mod.vignette",
      kind = "boolean",
      default = false,
      ui = { tab = "nowhere" },
    })
  end)
end)

test("a declaration that could hold nothing it allows raises", function()
  raises(function()
    trx.config.declare({
      key = "mod.outside",
      kind = "integer",
      default = 9,
      min = 0,
      max = 4,
    })
  end)
  raises(function()
    trx.config.declare({
      key = "mod.unlisted",
      kind = "dynamic_enum",
      values = { "one", "two" },
      default = "three",
    })
  end)
  raises(function()
    trx.config.declare({
      key = "mod.valueless",
      kind = "dynamic_enum",
      values = {},
      default = "one",
    })
  end)
end)

test("a key already taken raises", function()
  raises(function()
    trx.config.declare({
      key = "visuals.fov",
      kind = "integer",
      default = 1,
    })
  end)
end)

-- luaL_checkstring would take a number and rewrite the field into a string,
-- leaving the declaration reading a value that is no longer there.
test("a declaration names its fields with strings", function()
  raises(function()
    trx.config.declare({ key = 42, kind = "boolean", default = true })
  end)
end)

test(
  "a watcher hears the value it is attached to, and the ones after",
  function()
    local heard = {}
    local watcher = trx.config.on_change("visuals.fov", function(value)
      heard[#heard + 1] = value
    end)
    assert(heard[1] == 65, "a watcher must hear the value already in force")

    trx.config.set("visuals.fov", 90)
    assert(heard[2] == 90, "a watcher must hear a change")

    -- Another setting moving is not this watcher's business.
    trx.config.set("audio.enable_music", false)
    assert(#heard == 2, "a watcher heard a setting it does not watch")

    assert(watcher:detach() == true)
    trx.config.set("visuals.fov", 100)
    assert(#heard == 2, "a detached watcher still heard a change")
    assert(watcher:detach() == false, "a watcher is spent once detached")
  end
)

test("an override is a change like any other", function()
  local heard = {}
  local watcher = trx.config.on_change("visuals.fov", function(value)
    heard[#heard + 1] = value
  end)
  trx.config.override("visuals.fov", 120)
  assert(heard[#heard] == 120, "an override did not reach the watcher")

  trx.config.restore("visuals.fov")
  assert(heard[#heard] == 65, "restoring did not reach the watcher")
  watcher:detach()
end)

test("a watcher that raises does not silence the rest", function()
  local heard = false
  local angry = trx.config.on_change("visuals.fov", function()
    error("no")
  end)
  local calm = trx.config.on_change("visuals.fov", function()
    heard = true
  end)
  trx.config.set("visuals.fov", 90)
  assert(heard, "the second watcher did not run")
  angry:detach()
  calm:detach()
end)

test("a level script's watcher goes when the level does", function()
  local heard = 0
  fake.as_level_script(true)
  trx.config.on_change("visuals.fov", function()
    heard = heard + 1
  end)
  fake.as_level_script(false)
  local kept = trx.config.on_change("visuals.fov", function()
    heard = heard + 100
  end)

  fake.end_level()
  trx.config.set("visuals.fov", 90)
  -- Each watcher hears the value in force as it is attached, so the count
  -- starts at 101; only the one that outlived its level would raise it by one.
  assert(heard == 201, "the level's watcher outlived the level: " .. heard)
  kept:detach()
end)

-- A level script runs before the level is read, so a handler that reaches for
-- what the level carries would find nothing. The call it is owed waits for the
-- world instead.
test(
  "a level script's watcher hears the value once the level is read",
  function()
    local heard = {}
    fake.set_world_loaded(false)
    fake.as_level_script(true)
    local watcher = trx.config.on_change("visuals.fov", function(value)
      heard[#heard + 1] = value
    end)
    fake.as_level_script(false)

    assert(#heard == 0, "it was called before the level was read")
    fake.load_world()
    assert(#heard == 1, "it was not called once the level was read")

    watcher:detach()
    fake.end_level()
  end
)

test("a watcher attached after the level is read hears it at once", function()
  local heard = 0
  fake.as_level_script(true)
  local watcher = trx.config.on_change("visuals.fov", function()
    heard = heard + 1
  end)
  fake.as_level_script(false)

  assert(heard == 1, "the call did not come with the attach")
  watcher:detach()
  fake.end_level()
end)

test("a global script's watcher hears the value before any level", function()
  local heard = 0
  fake.set_world_loaded(false)
  local watcher = trx.config.on_change("visuals.fov", function()
    heard = heard + 1
  end)
  fake.set_world_loaded(true)

  assert(heard == 1, "a global watcher was made to wait")
  watcher:detach()
end)

-- A level that goes before it is ever read leaves its watchers owed a call
-- that must not arrive against the next level.
test("a watcher owed a call and dropped with its level stays quiet", function()
  local heard = 0
  fake.set_world_loaded(false)
  fake.as_level_script(true)
  trx.config.on_change("visuals.fov", function()
    heard = heard + 1
  end)
  fake.as_level_script(false)

  fake.end_level()
  fake.load_world()
  assert(heard == 0, "a dropped watcher was still called: " .. heard)
end)

return h.report()
