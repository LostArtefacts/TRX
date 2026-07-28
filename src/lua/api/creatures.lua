local raw = trxc.creatures
local api = trx.api

api.module("creatures", {
  order = 9,
  description = "Module for controlling certain creature behavior.",
})

api.property("creatures.hostile_allies", {
  type = "boolean",
  description = "Whether Lara's allies are hostile towards her.",
  get = raw.are_allies_hostile,
  set = raw.set_allies_hostile,
})

api.define("creatures.add_ally", {
  description = "Marks an object as an ally of Lara. Every item of that type becomes an ally.",
  params = {
    { name = "object_id", type = "integer", enum = "catalog.objects" },
  },
  examples = { [[trx.creatures.add_ally(trx.catalog.objects.monk_1)]] },
  impl = raw.add_ally,
})

api.define("creatures.add_ally_target", {
  description = "Marks an object as one that will fight any of Lara's allies. Every item of that "
    .. "type will target them.",
  params = {
    { name = "object_id", type = "integer", enum = "catalog.objects" },
  },
  examples = {
    [[trx.creatures.add_ally_target(trx.catalog.objects.bandit_1)]],
  },
  impl = raw.add_ally_target,
})
