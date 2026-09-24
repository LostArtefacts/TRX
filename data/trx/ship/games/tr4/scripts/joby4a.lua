trx.events.on_game_start(function()
  trx.objects.animating_13.properties.collidable = false
  trx.objects.animating_16.properties.collidable = false
  trx.items[89].properties.pickup_mode = trx.items.PickupMode.CROWBAR
  trx.items[177].properties.pickup_mode = trx.items.PickupMode.CROWBAR
end)

require("tr4.inv_setup").apply({
  key_item_11 = { scale = 1200, offset_y = 5, rot_x = -90 },
  key_item_12 = { scale = 768, offset_y = 4, rot_x = -90 },
  puzzle_item_5 = { scale = 1200, offset_y = 23, rot_y = 90 },
  puzzle_item_6 = { scale = 512, offset_y = 5, draws_at_pivot = true },
  puzzle_item_7 = { scale = 512, offset_y = 5, draws_at_pivot = true },
  puzzle_item_8 = { scale = 512, offset_y = 5, draws_at_pivot = true },
  pickup_item_1 = { scale = 1024, rot_x = 45, rot_z = 22.5 },
  pickup_item_2 = { scale = 1280, offset_y = 21, rot_x = -22.5, rot_y = 90 },
})
