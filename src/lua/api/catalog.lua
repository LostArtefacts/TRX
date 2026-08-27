local raw = trxc.catalog
local api = trx.api

api.module("catalog", {
  order = 8,
  description = "The names TRX knows things by.\n\n"
    .. "Each catalog is an enum of every object, sample, music track, Lara state, Lara animation "
    .. "or item action the engine has a name for. The names are the C ones with their prefix "
    .. "taken off - `O_WOLF` is `trx.catalog.objects.WOLF` - and a catalog answers to a name in "
    .. "any case, so `trx.catalog.objects.wolf` is the same constant.\n\n"
    .. "The ids in a catalog are TRX's own, and they are the same in all four games. The number a "
    .. "builder reads off Tomb Editor is not: that is the slot the game's own files use. "
    .. "`trx.catalog.to_slot` and `trx.catalog.from_slot` convert between the two.",
})

api.number("catalog.Id", {
  description = "A TRX id, in the catalog the context names. It is the same number in every "
    .. "game TRX ships, which is what lets a script name a thing once.",
})

api.number("catalog.Slot", {
  description = "A slot in this game's own files, which is the number a builder reads off Tomb "
    .. "Editor. It differs from game to game.",
})

local Context = api.enum("catalog.Context", {
  backing = "CATALOG_CONTEXT",
  strip = "CATALOG_",
  description = "Which catalog a slot belongs to.",
  values = {
    OBJECTS = "Objects.",
    MUSIC = "Music tracks.",
    SAMPLES = "Sound samples.",
    LARA_STATES = "Lara's states.",
    LARA_ANIMS = "Lara's animations.",
    ITEM_ACTIONS = "Item actions, which the flip effects trigger.",
  },
})

api.enum("catalog.objects", {
  backing = "OBJECT_ID",
  context = Context.OBJECTS,
  bulk = true,
  description = "Every object TRX has a name for.",
  examples = { [[if item.object_id == trx.catalog.objects.WOLF then ... end]] },
})

api.enum("catalog.samples", {
  backing = "SAMPLE_TRX_ID",
  context = Context.SAMPLES,
  bulk = true,
  description = "Every sound sample TRX has a name for.",
  examples = { [[trx.sound.play(trx.catalog.samples.LARA_NO)]] },
})

api.enum("catalog.music", {
  backing = "MUSIC_TRX_ID",
  context = Context.MUSIC,
  bulk = true,
  description = "Every music track TRX has a name for.",
  examples = { [[trx.music.play(trx.catalog.music.SECRET)]] },
})

api.enum("catalog.lara_states", {
  backing = "LARA_TRX_STATE",
  context = Context.LARA_STATES,
  bulk = true,
  description = "Every state Lara can be in.",
  examples = {
    [[if trx.lara.item.anim_state == trx.catalog.lara_states.RUN then ... end]],
  },
})

api.enum("catalog.lara_anims", {
  backing = "LARA_TRX_ANIMATION",
  context = Context.LARA_ANIMS,
  bulk = true,
  description = "Every animation Lara has.",
})

api.enum("catalog.flip_effects", {
  backing = "ITEM_TRX_ACTION",
  context = Context.ITEM_ACTIONS,
  bulk = true,
  description = "Every item action a flip effect can trigger.",
  examples = {
    [[trx.rooms.flip_effect(trx.catalog.flip_effects.FLOOR_SHAKE, 10)]],
  },
})

api.enum("catalog.weapons", {
  backing = "LARA_GUN_TYPE",
  strip = "LGT_",
  bulk = true,
  description = "Every weapon Lara can hold.",
  examples = {
    [[if trx.lara.equipped_gun == trx.catalog.weapons.DESERT_EAGLE then ... end]],
  },
})

api.define("catalog.key", {
  description = "Gives back the name an id answers to, which is the name a savegame stores "
    .. "and the name a mod writes. An id a script read out of the engine is a number, and this "
    .. "is what says which thing it names.",
  params = {
    {
      name = "context",
      type = "catalog.Context",
      description = "Which catalog.",
    },
    { name = "id", type = "catalog.Id" },
  },
  returns = {
    type = "string",
    nullable = true,
    description = "`nil` if the catalog holds no such id.",
  },
  examples = {
    [[local name = trx.catalog.key(trx.catalog.Context.OBJECTS, item.object_id)]],
  },
  impl = raw.key,
})

api.define("catalog.to_slot", {
  description = "Converts a `trx.catalog.Id` into the `trx.catalog.Slot` this game's own files "
    .. "use for it.",
  params = {
    {
      name = "context",
      type = "catalog.Context",
      description = "Which catalog.",
    },
    { name = "id", type = "catalog.Id" },
  },
  returns = {
    type = "catalog.Slot",
    nullable = true,
    description = "`nil` if this game has no slot for it - not every game has every object.",
  },
  examples = {
    [[local slot = trx.catalog.to_slot(trx.catalog.Context.OBJECTS, trx.catalog.objects.WOLF)]],
  },
  impl = raw.to_slot,
})

api.define("catalog.from_slot", {
  description = "Converts a `trx.catalog.Slot` from this game's own files into the "
    .. "`trx.catalog.Id` for it.",
  params = {
    {
      name = "context",
      type = "catalog.Context",
      description = "Which catalog.",
    },
    { name = "slot", type = "catalog.Slot" },
  },
  returns = {
    type = "catalog.Id",
    nullable = true,
    description = "`nil` if this game has nothing in that slot.",
  },
  examples = {
    [[local object_id = trx.catalog.from_slot(trx.catalog.Context.OBJECTS, 7)]],
  },
  impl = raw.from_slot,
})
