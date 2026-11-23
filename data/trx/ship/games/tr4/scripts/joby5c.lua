local cutscenes = require("tr4.cutscenes")

local CUTSCENE = 27
local CUTSCENE_ROOM = 55
local CUTSCENE_FLIP_GROUP = 8
local QUEST_ITEM_COUNT = 6

trx.events.on_game_start(function()
  trx.objects.scaled_spikes.properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.EXTENDED
end)

require("tr4.inv_setup").apply({
  key_item_11 = { scale = 1200, offset_y = 5, rot_x = -90 },
  key_item_12 = { scale = 768, offset_y = 4, rot_x = -90 },
})

-- Plays the scene only while Lara is in the room it plays in, as the
-- original game does.
trx.events.on_cutscene_trigger(function(num)
  if num ~= CUTSCENE or trx.lara.item.room_num == CUTSCENE_ROOM then
    return false
  end
  trx.cutscenes[num].is_played = false
  return true
end)

cutscenes.register(CUTSCENE, {
  on_start = function()
    -- Hides the closed shrine for good, as the scene draws its own copy and
    -- the open shrine replaces it once the scene ends.
    cutscenes.hide_items(trx.catalog.objects.animating_4)
    trx.cutscenes.set_lara_return({ x = 18982, y = 18176, z = 15841 }, -16184)
    local items =
      trx.items.query:of_object(trx.catalog.objects.animating_4):matches()
    for _, item in ipairs(items) do
      item.trigger_mask = 0
    end
    for i = 1, QUEST_ITEM_COUNT do
      trx.inventory.set_count(trx.catalog.objects["quest_item_" .. i], 0)
    end
  end,
  on_end = function()
    cutscenes.set_items_visible(trx.catalog.objects.animating_5, true)
    trx.rooms.flip(CUTSCENE_FLIP_GROUP)
  end,
})
