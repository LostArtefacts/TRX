local raw = trxc.inventory
local api = trx.api

api.module("inventory", {
  order = 4,
  description = [[
What Lara is carrying, and what goes into it.

The module is the inventory she holds now, so `trx.inventory:count(object)`
asks about her. Any level's is reached the same way through
`trx.game.Level.inventory`, which is what it will hand her when she arrives
there rather than what she has this second.

Every function takes either the pickup lying in the world or the inventory icon
it goes into. The engine maps one to the other, so a script names whichever it
has.]],
  instance = raw.get_current,
})

local object_param = {
  name = "object_id",
  type = "integer",
  enum = "catalog.objects",
  description = "The pickup, or the inventory icon it goes into.",
}

local count_param = {
  name = "count",
  type = "integer",
  optional = true,
  description = "How many. Defaults to 1; below 1 raises.",
}

local weapon_param = {
  name = "weapon",
  type = "integer",
  enum = "catalog.weapons",
  description = "Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the "
    .. "table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.",
}

api.type("inventory.Entry", {
  backing = "INVENTORY_ENTRY",
  description = [[
One kind of thing an inventory holds, and how many of it.

An entry stands for the icon rather than for where it sits, so it goes on
naming the same thing as what is drawn around it changes. A box of ammunition
is an entry like any other, counting what its rounds come to.]],

  fields = {
    object = {
      from = "object_id",
      type = "integer",
      enum = "catalog.objects",
      writable = false,
      description = "The inventory icon this entry is drawn as.",
    },
    count = {
      from = "qty",
      type = "integer",
      description = "How many of it there are. Writing 0 takes it away.",
    },
  },
})

api.type("inventory.Inventory", {
  backing = "INVENTORY_STATE",
  description = [[
An inventory: what is in it, and how much ammunition goes with it.

`trx.inventory` is the one Lara is carrying. A level's, reached as
`trx.game.Level.inventory`, is what she will arrive there with, and holds only
what travels between levels - a key or a puzzle piece belongs to the level it
was found in.

Giving something to Lara's does what walking over it would: a weapon arrives
with its rounds, her meshes change, and the level's own guns turn into
ammunition for it. Giving it to a level's only says what she will arrive
carrying.]],

  methods = {
    count = {
      description = "How many of something is in it. A box of ammunition counts what its rounds "
        .. "come to.",
      params = { object_param },
      returns = { type = "integer" },
    },
    set_count = {
      description = "Sets how many of it there are. Zero takes it away.",
      params = {
        object_param,
        {
          name = "count",
          type = "integer",
          description = "How many. Below 0 raises.",
        },
      },
    },
    has = {
      description = "Whether there is any of it at all.",
      params = { object_param },
      returns = { type = "boolean" },
    },
    give = {
      description = [[
Puts a pickup in. Lara's inventory takes it as walking over it would, so a
weapon arrives with the rounds a pickup carries and a flare box with its
flares; a level's simply gains it.]],
      params = { object_param, count_param },
      returns = {
        type = "integer",
        description = "How many went in. 0 from Lara's means the level does not carry the icon "
          .. "for it - see `can_add`.",
      },
      examples = { [[trx.inventory:give(trx.catalog.objects.uzi_item, 2)]] },
    },
    take = {
      description = [[
Takes things back out, stopping when there are none left.

This is not the exact opposite of `give`: a box of ammunition is rounds rather
than an entry of its own, so taking one back takes the rounds a box is worth.]],
      params = { object_param, count_param },
      returns = { type = "integer", description = "How many came out." },
    },
    shots = {
      description = "How many shots there are for the weapon. A shot is one pull of the trigger, "
        .. "which is what the counter shows the player; the shotgun spends six rounds on each.",
      params = { weapon_param },
      returns = { type = "integer" },
    },
    set_shots = {
      description = "Sets how many shots there are for it.",
      params = {
        weapon_param,
        {
          name = "count",
          type = "integer",
          description = "Shots. Below 0 raises.",
        },
      },
      examples = {
        [[trx.inventory:set_shots(trx.catalog.weapons.UZIS, 2000)]],
      },
    },
    has_weapon = {
      description = "Whether the weapon itself is in it, which is not the same as having "
        .. "ammunition for it.",
      params = { weapon_param },
      returns = { type = "boolean" },
    },
    entry = {
      description = [[
The entry something is drawn as, or `nil` where there is none of it.

Several pickups share one entry - the scion whether or not she holds it, a
waterskin at each fill level - so this answers with the one thing they are
drawn as.]],
      params = { object_param },
      returns = { type = "Entry", nullable = true },
    },
    entry_at = {
      description = "The entry at a position, counted from one in the order they are drawn, or "
        .. "`nil` past the end.",
      params = {
        { name = "n", type = "integer", description = "1-based position." },
      },
      returns = { type = "Entry", nullable = true },
    },
    entry_count = {
      description = "How many entries there are. `#trx.inventory` is the same number for the "
        .. "one Lara carries.",
      returns = { type = "integer" },
    },
    icon_of = {
      description = [[
Which inventory icon a pickup is drawn as, whether or not there is any of it.

Several pickups share one icon - the scion whether or not Lara holds it, a
waterskin at each fill level - so this is what tells two spellings of one thing
from two things. It answers with an object id rather than an entry; `entry` is
what hands back the entry itself.]],
      params = { object_param },
      returns = {
        type = "integer",
        enum = "catalog.objects",
        nullable = true,
        description = "The icon's object id, or `nil` for a pickup that has none.",
      },
    },
    can_add = {
      description = [[
Whether `give` would do anything in the level being played. The level has to
carry the inventory model, which is not the same as the pickup being in it: a
level with no shotgun lying about still draws one in the ring, which is what
lets a cheat hand one over.

This asks about the level being played whichever inventory it is called on.]],
      params = { object_param },
      returns = { type = "boolean" },
    },
  },
})

api.container("inventory", {
  description = "Indexing the module reaches an entry of Lara's inventory, and `#trx.inventory` "
    .. "is how many kinds of thing she carries. Entries count from one, in the order they are "
    .. "drawn, and are built one at a time as they are asked for. `pairs()` walks them.",
  base = 1,
  key = { type = "integer", description = "1-based position." },
  value = { type = "Entry", nullable = true },
  examples = {
    [[for _, entry in pairs(trx.inventory) do
  trx.log.info(("%d x %s"):format(entry.count, trx.catalog.objects[entry.object]))
end]],
  },
  get = function(n)
    return raw.get_current():entry_at(n)
  end,
  count = function()
    return raw.get_current():entry_count()
  end,
})
