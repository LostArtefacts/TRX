local raw = trxc.creatures
local h = require("trx.internal.helpers")

---@class trx
---@field creatures trx.creatures

---Module for controlling certain creature behavior.
---@trx.module 12
---@class (exact) trx.creatures
---@field hostile_allies boolean Whether Lara's allies are hostile towards her.
local M = h.module("creatures")

h.properties(M, "creatures", {
  hostile_allies = {
    get = raw.are_allies_hostile,
    set = raw.set_allies_hostile,
  },
})

---Marks an object as an ally of Lara. Every item of that type becomes an ally.
---
---```lua
---trx.creatures.add_ally(trx.catalog.objects.monk_1)
---```
---@param object_id trx.catalog.objects
---@type fun(object_id: trx.catalog.objects)
M.add_ally = raw.add_ally

---Marks an object as one that will fight any of Lara's allies. Every item of
---that type will target them.
---
---```lua
---trx.creatures.add_ally_target(trx.catalog.objects.bandit_1)
---```
---@param object_id trx.catalog.objects
---@type fun(object_id: trx.catalog.objects)
M.add_ally_target = raw.add_ally_target
