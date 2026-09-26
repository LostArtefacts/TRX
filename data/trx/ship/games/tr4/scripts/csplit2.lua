trx.events.on_game_start(function()
  trx.objects.switch_type_generic_1.properties.switch_mode =
    trx.items.SwitchMode.SHOVE
  trx.items[18].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS

  trx.objects.waterfall_1.properties.loop_sound =
    trx.items.WaterfallSound.WATER
  trx.objects.waterfall_2.properties.loop_sound =
    trx.items.WaterfallSound.WATER
  trx.objects.waterfall_3.properties.loop_sound =
    trx.items.WaterfallSound.WATER
end)

require("tr4.inv_setup").apply({
  puzzle_item_1 = { scale = 1024, offset_y = 20 },
  puzzle_item_2 = { scale = 1024, offset_y = 6, rot_x = -90 },
  puzzle_item_3 = { scale = 1280, draws_at_pivot = true },
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
