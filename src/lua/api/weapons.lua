local raw = trxc.weapons
local api = trx.api

api.module("weapons", {
  order = 5,
  description = [[
What a weapon is, rather than what Lara has of it.

None of this differs between the inventory she carries and the one a level
keeps for her, so it belongs to neither: what she holds and how many shots she
has are `trx.inventory`.]],
})

local weapon_param = {
  name = "weapon",
  type = "catalog.weapons",
  description = "Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the "
    .. "table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.",
}

api.define("weapons.is_available", {
  description = [[
Whether the game allows this weapon at all. The game flow can keep one out, and
a cheat that hands it over anyway leaves Lara with a gun the level was built
without.]],
  params = { weapon_param },
  returns = {
    type = "boolean",
    description = "True where this game has the weapon at all.",
  },
  impl = raw.is_available,
})

api.define("weapons.object", {
  description = "The pickup the weapon is, for handing it to `trx.inventory:give`.",
  params = { weapon_param },
  returns = {
    type = "catalog.objects",
    description = "The object id, or `nil` if this game has no such weapon.",
  },
  examples = {
    [[trx.inventory:give(trx.weapons.object(trx.catalog.weapons.SHOTGUN))]],
  },
  impl = raw.get_object,
})

api.define("weapons.shots_per_box", {
  description = "How many shots one box of ammunition for it is worth.",
  params = { weapon_param },
  returns = { type = "integer", description = "Shots, not rounds." },
  impl = raw.shots_per_box,
})
