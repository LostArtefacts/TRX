local raw = trxc.stats
local api = trx.api

api.module("stats", {
  order = 28,
  description = [[
Module for what a level keeps count of: what Lara has found in it, and how much
there was to find.

The module is the level being played, so `trx.stats.pickups.count` is what she
has picked up in it. Any other level's counters are reached the same way through
`trx.game.Level.stats`. At the title screen there is no level, and everything
here reads `nil`.]],
  instance = raw.get_current,
  instance_type = "stats.Stats",
})

-- The categories are ordered as the engine keeps them, so a name here stands
-- for the number the C side addresses one by.
local CATEGORY = {
  pickups = 0,
  kills = 1,
  secrets = 2,
  crystals = 3,
}

api.type("stats.Category", {
  backing = "STATS_CATEGORY",
  description = "One thing a level is counted on, which is one row of the statistics screen. `trx.stats.Category.raw` "
    .. "is `trx.stats.Category.max` plus `trx.stats.Category.unobtainable`: the game flow can declare part of a level out of reach, and "
    .. "what it writes off is left out of what counts towards completion while still being in the "
    .. "level.",

  fields = {
    count = {
      from = "count",
      type = "integer",
      description = "How many of them Lara has. The secrets cannot be set this way: they are held "
        .. "one by one, so `trx.stats.give_secret` and `trx.stats.take_secret` are how they change.",
    },
    max = {
      from = "max",
      type = "integer",
      writable = false,
      description = "How many of them count towards completing the level.",
    },
    raw = {
      from = "raw",
      type = "integer",
      writable = false,
      description = "How many of them the level holds, obtainable or not.",
    },
    unobtainable = {
      from = "unobtainable",
      type = "integer",
      writable = false,
      description = "How many of them the game flow declares out of reach, and so must not be held "
        .. "against the player.",
    },
  },
})

local function category(name, description)
  return {
    type = "stats.Category",
    description = description,
    impl = function(stats)
      return raw.category(stats, CATEGORY[name])
    end,
  }
end

api.number("stats.SecretNum", {
  base = 1,
  description = "The secret's number, as the player counts them.",
})

local secret_num_param = {
  name = "secret_num",
  type = "stats.SecretNum",
}

api.type("stats.Stats", {
  backing = "LEVEL_STATS",
  description = "What one level keeps count of. The counters are the level's own and can be "
    .. "written, which is what a script correcting or seeding them wants.",

  fields = {
    timer = {
      from = "timer",
      type = "game.Frames",
      description = "How long the level has been played.",
    },
    deaths = {
      from = "death_count",
      type = "integer",
      description = "How many times Lara has died. Unlike the rest, this is not cleared when the "
        .. "level is entered again: a death stays with the level it happened on.",
    },
    ammo_used = {
      from = "ammo_used",
      type = "integer",
      description = "How many rounds Lara has fired.",
    },
    ammo_hits = {
      from = "ammo_hits",
      type = "integer",
      description = "How many of them hit something.",
    },
    distance_travelled = {
      from = "distance_travelled",
      type = "math.Distance",
      description = "How far Lara has travelled.",
    },
    medipacks_used = {
      from = "medipacks_used",
      type = "number",
      description = "How many medipacks Lara has used, a small one counting as half of one.",
    },
  },

  extensions = {
    pickups = category(
      "pickups",
      "The items lying in the level for Lara to take."
    ),
    kills = category(
      "kills",
      "The enemies the level counts, allies among them."
    ),
    secrets = category(
      "secrets",
      "The level's secrets. Which ones Lara holds is `trx.stats.Stats.secret_list`."
    ),
    crystals = category(
      "crystals",
      "The save crystals, where the game has them."
    ),

    max_ally_kills = {
      type = "integer",
      description = "How many of `trx.stats.Stats.kills.max` are allies. The statistics screen holds them against "
        .. "the player only once `trx.stats.Stats.allies_hurt`, so a screen written in Lua wants to do the "
        .. "same: `trx.stats.Stats.max_enemy_kills`, and these as well once she has turned on one.",
      impl = function(stats)
        return (raw.kill_split(stats))
      end,
    },
    max_enemy_kills = {
      type = "integer",
      description = "How many of `trx.stats.Stats.kills.max` are enemies rather than allies.",
      impl = function(stats)
        return select(2, raw.kill_split(stats))
      end,
    },
    allies_hurt = {
      type = "boolean",
      description = "Whether Lara has turned on an ally in this level.",
      impl = raw.allies_hurt,
    },
  },

  methods = {
    secret_list = {
      description = "The level's secrets, in order.",
      returns = {
        type = "table",
        description = "The secrets, one by one.",
        list = true,
        fields = {
          {
            name = "num",
            type = "stats.SecretNum",
            description = "Which secret it is.",
          },
          {
            name = "found",
            type = "boolean",
            description = "Whether Lara has it.",
          },
        },
      },
      examples = {
        [[for _, secret in ipairs(trx.stats.secret_list()) do
  trx.log.info(secret.num .. ": " .. tostring(secret.found))
end]],
      },
    },
    give_secret = {
      description = "Marks a secret as found, as walking into its trigger would.",
      params = { secret_num_param },
      returns = {
        type = "boolean",
        description = "`false` if the level has no such secret, or Lara already has it.",
      },
      examples = { [[trx.stats.give_secret(1)]] },
    },
    take_secret = {
      description = "Takes a secret back, leaving it to be found again.",
      params = { secret_num_param },
      returns = {
        type = "boolean",
        description = "`false` if the level has no such secret, or Lara does not have it.",
      },
    },
  },
})
