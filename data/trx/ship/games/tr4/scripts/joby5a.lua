trx.events.on_game_start(function()
  trx.objects.animating_13.properties.collidable = false
  trx.objects.animating_14.properties.collidable = false
end)

require("tr4.inv_setup").apply({
  key_item_11 = { scale = 1200, offset_y = 5, rot_x = -90 },
  key_item_12 = { scale = 768, offset_y = 4, rot_x = -90 },
  puzzle_item_5 = { scale = 1200, offset_y = 23, rot_y = 90 },
  puzzle_item_6 = { scale = 512, offset_y = 5, draws_at_pivot = true },
  puzzle_item_7 = { scale = 512, offset_y = 5, draws_at_pivot = true },
  puzzle_item_8 = { scale = 512, offset_y = 5, draws_at_pivot = true },
  puzzle_item_9 = { scale = 512, offset_y = 5, draws_at_pivot = true },
})
