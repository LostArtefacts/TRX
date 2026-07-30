-- The config API as a script actually sees it.
--
-- The override stack underneath is the real one, so what these assert is the
-- thing that matters: a script can hold a setting away from the player's value
-- and give it back, without ever writing to their settings file.

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
    trx.config.get("visuals.water_color") == "ff0000",
    "a color stays a string"
  )
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

test("a string still works, which is what a color is", function()
  trx.config.set("visuals.water_color", "0080ff")
  assert(trx.config.get("visuals.water_color") == "0080ff")
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

test("a string option is given back by value", function()
  trx.config.override("visuals.water_color", "0080ff")
  assert(trx.config.get("visuals.water_color") == "0080ff")

  trx.config.restore("visuals.water_color")
  assert(
    trx.config.get("visuals.water_color") == "ff0000",
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
  assert(trx.config.is_overridden("visuals.fov") == false)
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
  assert(trx.config.describe("visuals.water_color").kind == "string")

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
  assert(all["visuals.water_color"] == "ff0000")
end)

return h.report()
