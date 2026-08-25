local raw = trxc.rules
local api = trx.api

api.module("rules", {
  order = 14,
  description = [[
    Module for the numbers the engine plays by.

    These are the game's rules: a mechanic that no single item owns. An
    object's own numbers live on the object, as
    `trx.objects.<name>.properties`, and the player's own choices live in
    `trx.config`.

    A rule lasts as long as the playthrough: it is saved with the game and
    restored with it, and a new game starts from the defaults. A level script
    states what its level wants, and states it again on every entry, so a level
    that wants the defaults back asks for them.
  ]],
})

-- Every rule is reachable two ways: as a member here, and by the dotted key
-- the console addresses it with. They are the same path, so the member is
-- spelled from the key rather than named a second time.
local declared = {}

local function rule(key, spec)
  spec.get = function()
    return raw.get(key)
  end
  spec.set = function(value)
    raw.set(key, value)
  end
  declared[key] = true
  api.property("rules." .. key, spec)
end

rule("exposure.max", {
  type = "game.Frames",
  description = "How much warmth Lara holds, and what `trx.lara.exposure_bar` fills "
    .. "to. Warmth only moves in a room carrying the `trx.rooms.Room.damaging` flag, such as the cold water of "
    .. "Antarctica.",
})

rule("exposure.drain_land", {
  type = "integer",
  description = "Warmth lost each frame in the cold, on land or wading.",
})

rule("exposure.drain_water", {
  type = "integer",
  description = "Warmth lost each frame in the cold, underwater or at the surface.",
})

rule("exposure.recovery", {
  type = "integer",
  description = "Warmth regained each frame once out of the cold.",
})

rule("exposure.damage", {
  type = "integer",
  description = "Hit points lost each frame once the warmth has run out.",
})

rule("corpse.fade_speed", {
  type = "integer",
  description = "How much of a body's coverage goes each frame, out of 255. It is taken "
    .. "away once nothing is left. `0` leaves it where it lies.",
})

rule("carrier.snap_to_sector", {
  type = "boolean",
  description = [[
    Whether an item a defeated enemy carried lands in the middle of the sector
    the enemy stood on, rather than at its feet. Quest items are left where
    they fall either way.
  ]],
})

rule("carrier.inherit_facing", {
  type = "boolean",
  description = [[
    Whether an item a defeated enemy carried turns to face the way the enemy
    did, rather than keeping the rotation the level gave it. This only reaches
    drops the level data places on the enemy; a drop the gameflow names always
    takes the enemy's facing.
  ]],
})

rule("fx.rotate_debris", {
  type = "boolean",
  description = [[
    Whether debris pieces generated from shattered meshes should rotate in yaw
    and pitch while they are active. The original TR4 did not apply rotation.
  ]],
})

api.define("rules.list", {
  description = "Every rule there is, as dotted `group.field` keys, in no particular order. "
    .. "<!--noref: group.field-->",
  params = {},
  returns = { type = "string", list = true },
  impl = raw.list,
})

api.define("rules.get", {
  description = "Reads a rule by its key, for code that does not know which one it wants.",
  params = {
    {
      name = "key",
      type = "string",
      description = "Dotted path, e.g. `exposure.damage`. <!--noref: exposure.damage-->",
    },
  },
  returns = { type = "any", description = "Raises if no rule has that key." },
  impl = raw.get,
})

api.define("rules.set", {
  description = "Changes a rule by its key. A string is read as text, the way the console gives "
    .. "it; any other value is taken as the rule's own type.",
  params = {
    {
      name = "key",
      type = "string",
      description = "Dotted path, e.g. `exposure.damage`. <!--noref: exposure.damage-->",
    },
    {
      name = "value",
      type = "any",
      description = "The value to write, of the type the rule declares.",
    },
  },
  impl = raw.set,
})

api.define("rules.reset", {
  description = "Puts a rule back to the value the engine ships with, or every rule when given "
    .. "no key. Happens on its own when a new game starts.",
  params = {
    {
      name = "key",
      type = "string",
      optional = true,
      description = "Dotted path.",
    },
  },
  impl = raw.reset,
})

api.define("rules.format_value", {
  description = "How a rule's value reads as text, for showing it to the player.",
  params = {
    { name = "key", type = "string", description = "Dotted path." },
  },
  returns = { type = "string", description = "The text, ready to print." },
  impl = raw.format_value,
})

-- A rule added to rules.def with no member here would be reachable by key and
-- absent from the reference; a member left behind would raise the first time a
-- script touched it. Neither survives boot.
local missing = {}
for _, key in ipairs(raw.list()) do
  if not declared[key] then
    missing[#missing + 1] = key
  end
  declared[key] = nil
end
for key in pairs(declared) do
  missing[#missing + 1] = key .. " (no such rule)"
end
if #missing > 0 then
  table.sort(missing)
  error("rules.lua does not declare: " .. table.concat(missing, ", "))
end
