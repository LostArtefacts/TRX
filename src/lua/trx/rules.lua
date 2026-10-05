local raw = trxc.rules
local h = require("trx.internal.helpers")

---@class (partial) trx
---@field rules trx.rules

---Module for the numbers the engine plays by.
---
---These are the game's rules: a mechanic that no single item owns. An
---object's own numbers live on the object, as
---`trx.objects.<name>.properties`, and the player's own choices live in
---`trx.config`.
---
---A rule lasts as long as the playthrough: it is saved with the game and
---restored with it, and a new game starts from the defaults. A level script
---states what its level wants, and states it again on every entry, so a level
---that wants the defaults back asks for them.
---@trx.module 15
---@class (exact) trx.rules
local M = h.module("rules")

-- Every rule is reachable two ways: as a member here, and by the dotted key
-- the console addresses it with. They are the same path, so the member is
-- spelled from the key rather than named a second time.
local declared = {}

local function rules(group, names)
  local props = {}
  for _, name in ipairs(names) do
    local key = group .. "." .. name
    props[name] = {
      get = function()
        return raw.get(key)
      end,
      set = function(value)
        raw.set(key, value)
      end,
    }
    declared[key] = true
  end
  h.properties(M[group], "rules." .. group, props)
end

---@class (exact) trx.rules.exposure
---@trx.implicit
---@field max trx.game.Frames How much warmth Lara holds, and what `trx.lara.exposure_bar` fills to. Warmth only moves in a room carrying the `trx.rooms.Room.damaging` flag, such as the cold water of Antarctica.
---@field drain_land integer Warmth lost each frame in the cold, on land or wading.
---@field drain_water integer Warmth lost each frame in the cold, underwater or at the surface.
---@field recovery integer Warmth regained each frame once out of the cold.
---@field damage integer Hit points lost each frame once the warmth has run out.
M.exposure = h.namespace("rules.exposure")
rules("exposure", { "max", "drain_land", "drain_water", "recovery", "damage" })

---@class (exact) trx.rules.corpse
---@trx.implicit
---@field fade_speed integer How much of a body's coverage goes each frame, out of 255. It is taken away once nothing is left. `0` leaves it where it lies.
M.corpse = h.namespace("rules.corpse")
rules("corpse", { "fade_speed" })

---@class (exact) trx.rules.carrier
---@trx.implicit
---@field snap_to_sector boolean Whether an item a defeated enemy carried lands in the middle of the sector the enemy stood on, rather than at its feet. Quest items are left where they fall either way.
---@field inherit_facing boolean Whether an item a defeated enemy carried turns to face the way the enemy did, rather than keeping the rotation the level gave it. This only reaches drops the level data places on the enemy; a drop the gameflow names always takes the enemy's facing.
M.carrier = h.namespace("rules.carrier")
rules("carrier", { "snap_to_sector", "inherit_facing" })

---@class (exact) trx.rules.inventory
---@trx.implicit
---@field keep_plot_items boolean Whether the items a level owns - keys, puzzle items, pickup items and what Lara examines - travel with her to the next level, rather than being left behind at the end of the one she found them in. TR4 keeps them and clears them where its game flow declares a `reset_hub` <!--noref: reset_hub-->; the other games leave them behind every time.
M.inventory = h.namespace("rules.inventory")
rules("inventory", { "keep_plot_items" })

---@class (exact) trx.rules.fx
---@trx.implicit
---@field rotate_debris boolean Whether debris pieces generated from shattered meshes should rotate in yaw and pitch while they are active. The original TR4 did not apply rotation.
M.fx = h.namespace("rules.fx")
rules("fx", { "rotate_debris" })

---Every rule there is, as dotted `group.field` keys, in no particular order.
---<!--noref: group.field-->
---@return string[]
---@type fun(): string[]
M.list = raw.list

---Reads a rule by its key, for code that does not know which one it wants.
---@param key string Dotted path, e.g. `exposure.damage`. <!--noref: exposure.damage-->
---@return any # Raises if no rule has that key.
---@type fun(key: string): any
M.get = raw.get

---Changes a rule by its key. A string is read as text, the way the console
---gives it; any other value is taken as the rule's own type.
---@param key string Dotted path, e.g. `exposure.damage`. <!--noref: exposure.damage-->
---@param value any The value to write, of the type the rule declares.
---@type fun(key: string, value: any)
M.set = raw.set

---Puts a rule back to the value the engine ships with, or every rule when
---given no key. Happens on its own when a new game starts.
---@param key? string Dotted path.
---@type fun(key?: string)
M.reset = raw.reset

---How a rule's value reads as text, for showing it to the player.
---@param key string Dotted path.
---@return string # The text, ready to print.
---@type fun(key: string): string
M.format_value = raw.format_value

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
