trx.events.on_game_start(function()
  trx.objects.animating_14.properties.collidable = false
  trx.objects.animating_15.properties.collidable = false
  trx.objects.animating_16.properties.collidable = false
  trx.items[42].properties.crowbar = true
  trx.items[79].properties.crowbar = true
  trx.items[82].properties.crowbar = true
  trx.items[83].properties.crowbar = true
  trx.items[36].properties.pickup_mode = trx.items.PickupMode.CROWBAR
  trx.objects.scaled_spikes.properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.ONE_SHOT
end)

require("tr4.inv_setup").apply({
  key_item_1 = { scale = 512, offset_y = 19, rot_y = 90 },
  key_item_10 = { scale = 768, offset_y = 8, rot_x = -45, rot_y = 180 },
  puzzle_item_1 = { scale = 1024, offset_y = 20 },
  puzzle_item_2 = { scale = 1024, offset_y = 6, rot_x = -90 },
  puzzle_item_3 = { scale = 1280, draws_at_pivot = true },
  puzzle_item_4 = { scale = 1792, offset_y = 2, rot_x = 45 },
  puzzle_item_5 = { scale = 1536, offset_y = 8, rot_x = -22.5, rot_y = 180 },
  puzzle_item_6 = { scale = 768 },
  puzzle_item_10 = { scale = 1024, offset_y = 17 },
  puzzle_item_11 = { scale = 1200, offset_y = 19 },
  puzzle_item_12 = {
    scale = 944,
    offset_y = 8,
    rot_x = -45,
    draws_at_pivot = true,
  },
  puzzle_item_4_combo_1 = {
    scale = 1024,
    offset_y = 10,
    rot_x = -90,
    rot_y = 67.5,
    draws_at_pivot = true,
  },
  puzzle_item_4_combo_2 = { scale = 1024, offset_y = 4, rot_x = -90 },
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
