local raw = trxc.catalog
local h = require("trx.internal.helpers")

---@class trx
---@field catalog trx.catalog

---The names TRX knows things by.
---
---Each catalog is an enum of every object, sample, music track, Lara state,
---Lara animation or item action the engine has a name for. The names are the C
---ones with their prefix taken off - `O_WOLF` is `trx.catalog.objects.WOLF` -
---and a catalog answers to a name in any case, so `trx.catalog.objects.wolf`
---is the same constant.
---
---The ids in a catalog are TRX's own, and they are the same in all four games.
---The number a builder reads off Tomb Editor is not: that is the slot the
---game's own files use. `trx.catalog.to_slot` and `trx.catalog.from_slot`
---convert between the two.
---@trx.module 9
---@class (exact) trx.catalog
local M = h.module("catalog")

---A TRX id, in the catalog the context names. It is the same number in every
---game TRX ships, which is what lets a script name a thing once.
---@alias trx.catalog.Id integer

---A slot in this game's own files, which is the number a builder reads off
---Tomb Editor. It differs from game to game.
---@alias trx.catalog.Slot integer

---Which catalog a slot belongs to.
---@enum trx.catalog.Context
local Context = {
  ---Objects.
  OBJECTS = h.IntegerConstant,
  ---Music tracks.
  MUSIC = h.IntegerConstant,
  ---Sound samples.
  SAMPLES = h.IntegerConstant,
  ---Lara's states.
  LARA_STATES = h.IntegerConstant,
  ---Lara's animations.
  LARA_ANIMS = h.IntegerConstant,
  ---Item actions, which the flip effects trigger.
  ITEM_ACTIONS = h.IntegerConstant,
  ---Weapons Lara can hold.
  WEAPONS = h.IntegerConstant,
  ---The families an object can belong to.
  FAMILIES = h.IntegerConstant,
}
M.Context = h.enum("catalog.Context", "CATALOG_CONTEXT", Context)

---Every object TRX has a name for.
---
---```lua
---if item.object_id == trx.catalog.objects.WOLF then ... end
---```
---@trx.bulk
---@trx.catalog objects.def O_
---@alias trx.catalog.objects integer

---@type table<string, trx.catalog.objects>
M.objects = h.catalog("catalog.objects", M.Context.OBJECTS)

---Every sound sample TRX has a name for.
---
---```lua
---trx.sound.play(trx.catalog.samples.LARA_NO)
---```
---@trx.bulk
---@trx.catalog samples.def SFX_
---@alias trx.catalog.samples integer

---@type table<string, trx.catalog.samples>
M.samples = h.catalog("catalog.samples", M.Context.SAMPLES)

---Every music track TRX has a name for.
---
---```lua
---trx.music.play(trx.catalog.music.SECRET)
---```
---@trx.bulk
---@trx.catalog music.def MX_
---@alias trx.catalog.music integer

---@type table<string, trx.catalog.music>
M.music = h.catalog("catalog.music", M.Context.MUSIC)

---Every state Lara can be in.
---
---```lua
---if trx.lara.item.anim_state == trx.catalog.lara_states.RUN then ... end
---```
---@trx.bulk
---@trx.catalog lara_states.def LS_
---@alias trx.catalog.lara_states integer

---@type table<string, trx.catalog.lara_states>
M.lara_states = h.catalog("catalog.lara_states", M.Context.LARA_STATES)

---Every animation Lara has.
---@trx.bulk
---@trx.catalog lara_anims.def LA_
---@alias trx.catalog.lara_anims integer

---@type table<string, trx.catalog.lara_anims>
M.lara_anims = h.catalog("catalog.lara_anims", M.Context.LARA_ANIMS)

---Every item action a flip effect can trigger.
---
---```lua
---trx.rooms.flip_effect(trx.catalog.flip_effects.FLOOR_SHAKE, 10)
---```
---@trx.bulk
---@trx.catalog item_actions.def ITEM_ACTION_
---@alias trx.catalog.flip_effects integer

---@type table<string, trx.catalog.flip_effects>
M.flip_effects = h.catalog("catalog.flip_effects", M.Context.ITEM_ACTIONS)

---Every weapon Lara can hold.
---
---```lua
---if trx.lara.equipped_gun == trx.catalog.weapons.DESERT_EAGLE then ... end
---```
---@trx.bulk
---@trx.catalog weapons.def LGT_
---@alias trx.catalog.weapons integer

---@type table<string, trx.catalog.weapons>
M.weapons = h.catalog("catalog.weapons", M.Context.WEAPONS)

---Declares an identity the engine has no constant for, and gives back its id.
---This is how a mod that ships only a script introduces an object of its own,
---without touching the catalog the game owns.
---
---The identity carries no slot, because nothing in the game's own files
---refers to it. It lasts until the mod is unloaded.
---
---A savegame records an object by the slot this game's files use, so an item
---of a minted object is not written to one and does not come back on load.
---Spawn it from a script until savegames record a name.
---
---```lua
---local drum = trx.catalog.mint(trx.catalog.Context.OBJECTS, "oil_drum")
---```
---@param context trx.catalog.Context Which catalog.
---@param name string The name to declare. Letters, digits and `:_-`, so a mod can put its own prefix in front of what it brings.
---@return trx.catalog.Id? # `nil` if the name is not one an identity may take, or the catalog already holds it.
---@type fun(context: trx.catalog.Context, name: string): trx.catalog.Id?
M.mint = raw.mint

---Gives back the name an id answers to, which is the name a savegame stores
---and the name a mod writes. An id a script read out of the engine is a
---number, and this is what says which thing it names.
---
---```lua
---local name = trx.catalog.key(trx.catalog.Context.OBJECTS, item.object_id)
---```
---@param context trx.catalog.Context Which catalog.
---@param id trx.catalog.Id
---@return string? # `nil` if the catalog holds no such id.
---@type fun(context: trx.catalog.Context, id: trx.catalog.Id): string?
M.key = raw.key

---Converts a `trx.catalog.Id` into the `trx.catalog.Slot` this game's own
---files use for it.
---
---```lua
---local slot = trx.catalog.to_slot(trx.catalog.Context.OBJECTS, trx.catalog.objects.WOLF)
---```
---@param context trx.catalog.Context Which catalog.
---@param id trx.catalog.Id
---@return trx.catalog.Slot? # `nil` if this game has no slot for it - not every game has every object.
---@type fun(context: trx.catalog.Context, id: trx.catalog.Id): trx.catalog.Slot?
M.to_slot = raw.to_slot

---Converts a `trx.catalog.Slot` from this game's own files into the
---`trx.catalog.Id` for it.
---
---```lua
---local object_id = trx.catalog.from_slot(trx.catalog.Context.OBJECTS, 7)
---```
---@param context trx.catalog.Context Which catalog.
---@param slot trx.catalog.Slot
---@return trx.catalog.Id? # `nil` if this game has nothing in that slot.
---@type fun(context: trx.catalog.Context, slot: trx.catalog.Slot): trx.catalog.Id?
M.from_slot = raw.from_slot
