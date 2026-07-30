local raw = trxc.stats
local api = trx.api

api.module("stats", {
  order = 22,
  description = [[
Module for what the level being played keeps count of: what Lara has found in
it, and how much there was to find.

The counts belong to the level, not to the session. Nothing here reads outside
one, so a script that runs at the title screen sees an empty list and zero
counts.]],
})

api.property("stats.secrets", {
  type = "table",
  description = [[
The secrets the level holds, in order, as a list of `{ num, found }`. `num` is
the number the player says, counted from one, and `found` is whether Lara has
it.]],
  get = raw.secrets,
})

api.property("stats.secret_count", {
  type = "integer",
  description = "How many secrets Lara has found in this level.",
  get = raw.secret_count,
})

api.property("stats.max_secret_count", {
  type = "integer",
  description = [[
How many secrets the level counts towards completion. Not the length of
`secrets`: the game flow can declare some of a level's secrets unobtainable,
and those are left out of this count while still standing in the list.]],
  get = raw.max_secret_count,
})

local num_param = {
  name = "num",
  type = "integer",
  description = "The secret's number, counted from one.",
}

api.define("stats.give_secret", {
  description = "Marks a secret as found, as walking into its trigger would.",
  params = { num_param },
  returns = {
    type = "boolean",
    description = "`false` if the level has no such secret, or Lara already has it.",
  },
  examples = { [[trx.stats.give_secret(1)]] },
  impl = raw.give_secret,
})

api.define("stats.take_secret", {
  description = "Takes a secret back, leaving it to be found again.",
  params = { num_param },
  returns = {
    type = "boolean",
    description = "`false` if the level has no such secret, or Lara does not have it.",
  },
  impl = raw.take_secret,
})
