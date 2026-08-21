local raw = trxc.store
local api = trx.api

api.module("store", {
  order = 20,
  title = "Persistent storage",
  description = [[
Module for what a script remembers across a save.

Two plain tables. What a script writes into either one is written into the
savegame and read back on load, so a script keeps its state across a save. The
tables themselves never change identity, so `local s = trx.store.level` taken
once still names the store after a load.

A store holds numbers, strings, booleans and tables of those. A value of any
other type, a function among them, is dropped when the game is saved and the
key it sat at is named in the log. A value is checked as the game is written
rather than as a script assigns it, so a value a save cannot hold is reported
later than the line that stored it.

Put one table in two places and you get one table back, not two copies. Change
it through the first place and the second place shows the change. A table that
refers to itself, directly or through another table, works too.

An item handle, and any other handle the engine owns, is not a value a store
takes. A script that wants to name an item across a save should store something
the level carries, such as the item's index.
]],
})

api.property("store.level", {
  type = "table",
  description = "What the level remembers. Emptied when a level loads, so a script that wants a "
    .. "starting state writes it from `trx.events.on_game_start`.",
  get = function()
    return raw.level
  end,
})

api.property("store.game", {
  type = "table",
  description = "What the playthrough remembers. It survives a level change and is emptied only "
    .. "when a new game starts.",
  get = function()
    return raw.game
  end,
})
