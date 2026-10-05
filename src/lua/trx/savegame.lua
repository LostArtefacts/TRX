local raw = trxc.savegame
local api = trx.api

api.module("savegame", {
  order = 28,
  description = "Manage save slots and saved games.",
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
  description = [[Counts the slots in a pool.

The quick pool counts only slots that hold a save.]],
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
  description = [[Starts the saved game in a slot.

The game flow loads it after this call returns. Raises when the slot holds no
save.]],
  params = { slot_param, pool_param },
  examples = { [[trx.savegame.load(1)]] },
  impl = function(slot_num, pool)
    raw.load(slot_num, pool or Pool.NORMAL)
  end,
})

api.define("savegame.save", {
  description = [[Writes a saved game to a slot.

A quick save without a slot number uses the next slot in the rotation. Otherwise,
it uses the named slot.]],
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
      description = "Whether the save was written. `false` means that the quick pool had no slot.",
    },
  },
  examples = { [[trx.savegame.save(1)]] },
  impl = function(slot_num, pool)
    return raw.save(slot_num, pool or Pool.NORMAL)
  end,
})

api.type("savegame.SlotInfo", {
  description = "What a slot holds, as the save list shows it.",
  fields = {
    level_title = {
      type = "string",
      description = "The name of the level the save was made in.",
    },
    counter = {
      type = "integer",
      description = "The save count when this save was written.",
    },
    level_num = {
      type = "integer",
      description = "Where the level sits in the main level table.",
    },
    is_quick = {
      type = "boolean",
      description = "Whether the save is a quick save.",
    },
    can_restart = {
      type = "boolean",
      description = "Whether the level can be restarted from the save.",
    },
    can_select_level = {
      type = "boolean",
      description = "Whether the save reaches an earlier level in the game.",
    },
    has_story = {
      type = "boolean",
      description = "Whether story content runs before the saved level.",
    },
  },
})

api.define("savegame.info", {
  description = "Returns the slot contents, or `nil` if the slot is empty.",
  params = { slot_param, pool_param },
  returns = {
    type = "savegame.SlotInfo",
    nullable = true,
    description = "What the slot holds.",
  },
  examples = {
    [[local info = trx.savegame.info(1)
if info ~= nil then
  trx.log.info(info.level_title)
end]],
  },
  impl = function(slot_num, pool)
    return raw.info(slot_num, pool or Pool.NORMAL)
  end,
})

api.define("savegame.delete", {
  description = "Removes the save from a slot and deletes its file.",
  params = { slot_param, pool_param },
  returns = {
    { type = "boolean", description = "Whether a save was removed." },
  },
  impl = function(slot_num, pool)
    return raw.delete(slot_num, pool or Pool.NORMAL)
  end,
})

api.define("savegame.total_count", {
  description = "Counts the saves in every pool.",
  returns = { { type = "integer", description = "The number of saves." } },
  impl = raw.total_count,
})

api.define("savegame.restart_available", {
  description = [[Reports whether the saved level can be restarted.

With no slot, this uses the save the game is running from. A game that is not
running from a save can always restart.]],
  params = {
    {
      name = "slot_num",
      type = "savegame.SlotNum",
      optional = true,
      description = "The slot to ask about. The running save answers when it is omitted.",
    },
    pool_param,
  },
  returns = {
    { type = "boolean", description = "Whether the level can be restarted." },
  },
  impl = function(slot_num, pool)
    return raw.restart_available(
      slot_num,
      slot_num ~= nil and (pool or Pool.NORMAL) or nil
    )
  end,
})

api.define("savegame.reached_levels", {
  description = [[Returns the levels up to the saved one without starting it.]],
  params = { slot_param, pool_param },
  returns = {
    {
      type = "game.LevelNum",
      list = true,
      nullable = true,
      description = "The levels, or `nil` when the slot holds no save that can be read.",
    },
  },
  impl = function(slot_num, pool)
    return raw.reached_levels(slot_num, pool or Pool.NORMAL)
  end,
})

api.define("savegame.play_story", {
  description = [[Plays the story content that runs before the saved level.

Raises when the slot holds no save, or when no story runs before it.]],
  params = { slot_param, pool_param },
  impl = function(slot_num, pool)
    raw.play_story(slot_num, pool or Pool.NORMAL)
  end,
})

api.define("savegame.recent_slot", {
  description = [[Returns the slot where a save list should open.

This is the slot that the game last loaded or saved. If there is no such slot,
it is the most recently written save, and then the first numbered slot.]],
  returns = {
    {
      type = "savegame.SlotNum",
      nullable = true,
      description = "The slot number, or `nil` where the game keeps no slots.",
    },
    { type = "savegame.Pool", description = "Which pool it belongs to." },
  },
  impl = raw.recent_slot,
})

api.property("savegame.manual_allowed", {
  type = "boolean",
  description = "Whether the current level allows manual saving.",
  get = raw.manual_allowed,
})
