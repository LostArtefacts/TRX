local raw = trxc.savegame
local api = trx.api

api.module("savegame", {
  order = 26,
  description = "The save slots, and starting or reading a saved game.",
})

local Pool = api.enum("savegame.Pool", {
  backing = "SAVEGAME_SLOT_POOL",
  description = "Which set of save slots a slot belongs to.",
  values = {
    NORMAL = "The numbered save slots.",
    QUICK = "The quick-save slots, counted and addressed by their on-screen order.",
  },
})

local pool_param = {
  name = "pool",
  type = "savegame.Pool",
  optional = true,
  description = "Which set of slots to look in. Defaults to `NORMAL`.",
}

api.number("savegame.SlotNum", {
  base = 1,
  description = "Slot number within the pool. For the quick pool this is the "
    .. "on-screen order.",
})

local slot_param = {
  name = "slot_num",
  type = "savegame.SlotNum",
}

api.define("savegame.slot_count", {
  description = "How many slots a pool has. The quick pool counts only the slots that hold a "
    .. "save, which is how it is shown and addressed.",
  params = { pool_param },
  returns = { { type = "integer", description = "The number of slots." } },
  impl = function(pool)
    return raw.slot_count(pool or Pool.NORMAL)
  end,
})

api.define("savegame.is_free", {
  description = "Whether a slot holds no save.",
  params = { slot_param, pool_param },
  returns = {
    { type = "boolean", description = "Whether the slot is empty." },
  },
  impl = function(slot_num, pool)
    return raw.is_free(slot_num, pool or Pool.NORMAL)
  end,
})

api.define("savegame.load", {
  description = "Starts the saved game in a slot. The load happens once the game flow picks it "
    .. "up, not on the call.",
  params = { slot_param, pool_param },
  examples = { [[trx.savegame.load(1)]] },
  impl = function(slot_num, pool)
    raw.load(slot_num, pool or Pool.NORMAL)
  end,
})

api.define("savegame.save", {
  description = "Writes a saved game to a slot. A quick save with no slot number goes to the next "
    .. "slot in the rotation; with one, it saves to the slot named.",
  params = {
    {
      name = "slot_num",
      type = "savegame.SlotNum",
      optional = true,
      description = "The quick pool uses the next slot in its rotation "
        .. "when it is omitted.",
    },
    pool_param,
  },
  returns = {
    {
      type = "boolean",
      description = "Whether the save was written. `false` means the quick pool had no slot.",
    },
  },
  examples = { [[trx.savegame.save(1)]] },
  impl = function(slot_num, pool)
    return raw.save(slot_num, pool or Pool.NORMAL)
  end,
})
