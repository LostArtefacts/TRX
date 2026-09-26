trx.events.on_game_start(function()
  trx.items[16].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[33].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[34].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[35].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[70].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[71].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW

  trx.objects.animating_16.properties.collidable = false
end)

require("tr4.inv_setup").apply({
  key_item_2 = { scale = 1024, rot_x = -90 },
  key_item_3 = { scale = 1024, rot_x = -90 },
  puzzle_item_1 = { scale = 1280 },
  puzzle_item_2 = { scale = 800, offset_y = 1 },
  puzzle_item_3 = { scale = 800, offset_y = 1 },
  puzzle_item_1_combo_1 = { scale = 384 },
  puzzle_item_1_combo_2 = { scale = 1200 },
})
