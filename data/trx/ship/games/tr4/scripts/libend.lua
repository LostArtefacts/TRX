local cutscenes = require("tr4.cutscenes")

local VON_CROY_CUTSCENE = 10

trx.events.on_game_start(function()
  trx.items[13].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
end)

require("tr4.inv_setup").apply({
  puzzle_item_5 = { scale = 1536, offset_y = 8, rot_x = -22.5, rot_y = 180 },
  puzzle_item_10 = { scale = 1024, offset_y = 17 },
  puzzle_item_11 = { scale = 1200, offset_y = 19 },
  puzzle_item_12 = {
    scale = 944,
    offset_y = 8,
    rot_x = -45,
    draws_at_pivot = true,
  },
  puzzle_item_5_combo_1 = {
    scale = 1280,
    offset_y = 2,
    rot_x = 22.5,
    rot_y = 90,
    draws_at_pivot = true,
  },
  puzzle_item_5_combo_2 = { scale = 1024, offset_y = 22, rot_y = 180 },
  pickup_item_1 = {
    scale = 944,
    offset_y = 8,
    rot_x = -45,
    draws_at_pivot = true,
  },
  pickup_item_2 = { scale = 336, offset_y = 8 },
})

cutscenes.register(VON_CROY_CUTSCENE, {
  hide = { trx.catalog.objects.door_5 },
  hidden_actors = { 1, 2, 3, 4 },
  actor_cues = {
    [906] = { show = { 1, 2, 3, 4 } },
    [1205] = { hide = { 1, 2, 3, 4 } },
    [1316] = { show = { 1, 2, 3, 4 } },
    [1747] = { hide = { 1, 2, 3, 4 } },
    [2007] = { show = { 1, 2, 3, 4 } },
    [2241] = { hide = { 2, 3, 4 } },
  },
  on_start = function()
    trx.cutscenes.set_lara_return({ x = 68756, y = 512, z = 56493 }, 983)
  end,
})
