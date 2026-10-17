-- The entries of the shipped inventory ring: how each is declared, and which
-- ring each goes on.

local h = require("harness")
local test = h.test

local entries = require("common.ui.ring.entries")
local Ring = entries.Ring
local objects = trx.catalog.objects
local GAME = trx.ui.MenuMode.GAME
local TITLE = trx.ui.MenuMode.TITLE

local function declare(object, inv_pos)
  entries.declare({
    object_id = trx.catalog.key(trx.catalog.Context.OBJECTS, object),
    inv_pos = inv_pos,
  })
end

local function objects_of(list)
  local result = {}
  for i, content in ipairs(list) do
    result[i] = content.object
  end
  return result
end

local function same(a, b)
  if #a ~= #b then
    return false
  end
  for i = 1, #a do
    if a[i] ~= b[i] then
      return false
    end
  end
  return true
end

declare(objects.PISTOLS_OPTION, 1)
declare(objects.SMALL_MEDIPACK_OPTION, 14)
declare(objects.SHOTGUN_OPTION, 2)
declare(objects.KEY_OPTION_1, 110)
for i, object in ipairs({
  objects.PASSPORT_CLOSED,
  objects.GRAPHICS_OPTION,
  objects.CONTROLS_OPTION,
  objects.SOUND_OPTION,
  objects.PDA_OPTION,
  objects.PHOTO_OPTION,
}) do
  declare(object, 200 + i)
end

test("a later declaration changes only the fields it names", function()
  entries.declare({ object_id = "pistols_option", z_trans_sel = 300 })
  entries.declare({ object_id = "pistols_option", scale = 2 })
  local item = entries.item(objects.PISTOLS_OPTION)
  assert(item ~= nil and item.z_trans_sel == 300 and item.scale == 2)
  assert(item.inv_pos == 1, "the place it was declared at went")
end)

test("an entry that names no speed shows each frame for one step", function()
  entries.declare({ object_id = "shotgun_option" })
  assert(entries.item(objects.SHOTGUN_OPTION).anim_speed == 0)
end)

test("an unknown object cannot be declared", function()
  h.raises(function()
    entries.declare({ object_id = "no_such_object" })
  end, "unknown object")
end)

test("the main ring holds what Lara carries, in the declared order", function()
  trx.inventory.give(objects.SMALL_MEDIPACK_OPTION, 3)
  trx.inventory.give(objects.SHOTGUN_OPTION)
  trx.inventory.give(objects.PISTOLS_OPTION)
  trx.inventory.give(objects.KEY_OPTION_1)
  local main = entries.contents(Ring.MAIN, GAME)
  assert(same(objects_of(main), {
    objects.PISTOLS_OPTION,
    objects.SHOTGUN_OPTION,
    objects.SMALL_MEDIPACK_OPTION,
  }))
  assert(main[3].count == 3, "the count was lost")
  assert(same(objects_of(entries.contents(Ring.KEYS, GAME)), {
    objects.KEY_OPTION_1,
  }))
end)

test("the menu ring sits in the order it is declared in", function()
  fake.set_gym_present(false)
  fake.set_tr_version(1)
  assert(same(objects_of(entries.contents(Ring.OPTION, GAME)), {
    objects.PASSPORT_CLOSED,
    objects.GRAPHICS_OPTION,
    objects.CONTROLS_OPTION,
    objects.SOUND_OPTION,
    objects.PDA_OPTION,
  }))
end)

test("entries declared in one place keep each game's order", function()
  fake.set_gym_present(false)
  for _, object in ipairs({
    objects.GRAPHICS_OPTION,
    objects.CONTROLS_OPTION,
    objects.SOUND_OPTION,
  }) do
    declare(object, 202)
  end
  fake.set_tr_version(1)
  local tr1 = objects_of(entries.contents(Ring.OPTION, GAME))
  fake.set_tr_version(2)
  local tr2 = objects_of(entries.contents(Ring.OPTION, GAME))
  declare(objects.CONTROLS_OPTION, 203)
  declare(objects.SOUND_OPTION, 204)
  assert(same(tr1, {
    objects.PASSPORT_CLOSED,
    objects.CONTROLS_OPTION,
    objects.SOUND_OPTION,
    objects.GRAPHICS_OPTION,
    objects.PDA_OPTION,
  }))
  assert(same(tr2, {
    objects.PASSPORT_CLOSED,
    objects.GRAPHICS_OPTION,
    objects.CONTROLS_OPTION,
    objects.SOUND_OPTION,
    objects.PDA_OPTION,
  }))
end)

test("the title's menu offers the gym where the game has one", function()
  fake.set_tr_version(2)
  fake.set_gym_present(true)
  local title = objects_of(entries.contents(Ring.OPTION, TITLE))
  local game = objects_of(entries.contents(Ring.OPTION, GAME))
  fake.set_gym_present(false)
  assert(title[#title] == objects.PHOTO_OPTION, "the title had no gym")
  assert(game[#game] ~= objects.PHOTO_OPTION, "the game offered the gym")
end)

test("a ring with nothing on it is not available", function()
  assert(not entries.is_available(Ring.KEYS, GAME))
  trx.inventory.give(objects.KEY_OPTION_1)
  assert(entries.is_available(Ring.KEYS, GAME))
end)

return h.report()
