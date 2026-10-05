local raw = trxc.mod
local h = require("trx.internal.helpers")

---@class trx
---@field mod trx.mod

---The mods the game was built with, and which one is loaded.
---@trx.module 30
---@class (exact) trx.mod
---@trx.readonly current, list
---@field list trx.mod.Mod[] The mods the game was built with, counted from one.
---@field current trx.mod.Mod The loaded mod.
local M = h.module("mod")

---What kind of mod it is.
---@enum trx.mod.Type
local Type = {
  BASE_GAME = "The base game.",
  EXPANSION_PACK = "An expansion pack.",
  MISC = "A miscellaneous mod.",
  DIRECT_LEVEL = "A single level loaded on its own.",
  CUSTOM = "A custom mod.",
}
M.Type = h.enum("mod.Type", "SHELL_MOD_TYPE", Type)

---A mod the game can run. Everything on it is read-only.
---@class (exact) trx.mod.Mod
---@trx.readonly base_mod, can_switch, engine_version, is_available, is_valid,
---  name, title, type
---@field name string The mod's identifier, as `trx.mod.switch` takes it.
---@field title string The mod's name, as shown to the player.
---@field type trx.mod.Type What kind of mod it is.
---@field engine_version integer Which Tomb Raider the mod runs on.
---@field base_mod string? The mod this one builds on, or `nil` if it stands alone.
---@field is_available boolean Whether the mod's files are present.
---@field is_valid boolean Whether the mod can be loaded.
---@field can_switch boolean Whether `trx.mod.switch` accepts the mod. A single level loaded on its own is valid, but is not a mod to switch to.
local Mod = h.handle("mod.Mod", "SHELL_MOD", {
  fields = {
    name = "name",
    title = "title",
    type = "mod_type",
    engine_version = "engine_version",
    base_mod = "base_mod",
    is_available = "is_available",
    is_valid = "is_valid",
    can_switch = "can_switch",
  },
})

h.properties(M, "mod", {
  list = {
    get = function()
      local mods = {}
      for i = 1, raw.count() do
        mods[i] = raw.get(i - 1)
      end
      return mods
    end,
  },
  current = {
    get = raw.get_current,
  },
})

---Whether the game can restart into a mod.
---
---Incompatible and current mods cannot be switched to.
---@param mod any A `trx.mod.Mod` or a mod name.
---@return boolean # Whether the mod can be switched to.
---@type fun(mod: any): boolean
M.can_switch = raw.can_switch

---Restarts the game into another mod. The switch happens once the game flow
---picks it up, not on the call.
---
---```lua
---trx.mod.switch("arabian-nights")
---```
---@param mod any A `trx.mod.Mod` or a mod name.
---@return boolean # Whether the mod can be switched to. `false` leaves the game where it is.
---@type fun(mod: any): boolean
M.switch = raw.switch
