trx.events.on_game_start(function()
  trx.items[45].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW

  trx.objects.animating_13.properties.collidable = false
  trx.objects.animating_14.properties.collidable = false
  trx.objects.animating_16.properties.collidable = false
  trx.objects.switch_type_generic_1.properties.switch_mode =
    trx.items.SwitchMode.SHOVE
end)

require("tr4.inv_setup").apply({
  puzzle_item_1 = { scale = 768, offset_y = 4 },
  puzzle_item_2 = { scale = 1024, rot_x = 180, rot_y = 90, rot_z = 90 },
  puzzle_item_4 = { scale = 768, rot_x = -67.5, rot_y = 90, rot_z = -112.5 },
  puzzle_item_5 = { scale = 512, rot_x = -90, rot_y = 180 },
  puzzle_item_8 = { scale = 1024, offset_y = 8, rot_y = 180 },
  puzzle_item_1_combo_1 = { scale = 768, offset_y = 4, draws_at_pivot = true },
  puzzle_item_1_combo_2 = { scale = 768, offset_y = 2 },
  puzzle_item_2_combo_1 = { scale = 1024, rot_x = 180, rot_y = 90, rot_z = 90 },
  puzzle_item_2_combo_2 = { scale = 768, rot_x = 180, rot_y = 90, rot_z = 90 },
  puzzle_item_8_combo_1 = { scale = 1024, offset_y = 8, rot_y = 180 },
  puzzle_item_8_combo_2 = {
    scale = 640,
    offset_y = 4,
    rot_x = -90,
    rot_y = 180,
  },
})
