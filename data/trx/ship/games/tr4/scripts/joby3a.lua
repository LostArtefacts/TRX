trx.creatures.add_ally(trx.catalog.objects.soldier_1)

local cutscenes = require("tr4.cutscenes")

-- The big scorpion plays this scene when Lara kills it while it holds the
-- soldier. As in the original game, the end of the scene gives Lara two keys
-- and starts the animating 9 items in the scorpion's room.
local SCORPION_CUTSCENE = 4
local SCORPION_ROOM = 101

local function open_the_way()
  trx.inventory.set_count(trx.catalog.objects.key_item_11, 1)
  trx.inventory.set_count(trx.catalog.objects.key_item_12, 1)

  local query = trx.items.query:where(function(_, item)
    return item.room_num == SCORPION_ROOM
      and item.object_id == trx.catalog.objects.animating_9
  end)
  for _, item in ipairs(query:matches()) do
    item:trigger()
  end
end

cutscenes.register(SCORPION_CUTSCENE, {
  chat = {
    {
      actor = 1,
      node = 8,
      ranges = {
        { 485, 547 },
        { 558, 625 },
        { 639, 652 },
        { 661, 721 },
        { 729, 793 },
        { 805, 872 },
      },
      speech_heads = {
        trx.catalog.objects.actor_1_speech_head_1,
        trx.catalog.objects.actor_1_speech_head_2,
      },
    },
  },
  on_end = open_the_way,
})

require("tr4.inv_setup").apply({
  key_item_11 = { scale = 1200, offset_y = 5, rot_x = -90 },
  key_item_12 = { scale = 768, offset_y = 4, rot_x = -90 },
  puzzle_item_5 = { scale = 1200, offset_y = 23, rot_y = 90 },
  puzzle_item_6 = { scale = 512, offset_y = 5, draws_at_pivot = true },
})
