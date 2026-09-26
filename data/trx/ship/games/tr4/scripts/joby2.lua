trx.events.on_game_start(function()
  trx.objects.animating_16.properties.collidable = false
  trx.items[116].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[118].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[120].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[122].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[123].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[124].properties.switch_mode = trx.items.SwitchMode.HIDDEN_PICKUP
  trx.items[125].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[126].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[127].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[128].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[129].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[130].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[131].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[139].properties.pickup_mode = trx.items.PickupMode.HIDDEN
  trx.objects.scaled_spikes.properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.EXTENDED
end)

require("tr4.inv_setup").apply({
  key_item_11 = { scale = 1200, offset_y = 5, rot_x = -90 },
  key_item_12 = { scale = 768, offset_y = 4, rot_x = -90 },
  puzzle_item_1 = { scale = 1280, offset_y = 2, rot_x = -90 },
  puzzle_item_2 = { scale = 1280, offset_y = 2, rot_x = -90 },
  puzzle_item_3 = { scale = 1280, offset_y = 2, rot_x = -90 },
  puzzle_item_4 = { scale = 1280, offset_y = 2, rot_x = -90 },
  puzzle_item_5 = { scale = 1200, offset_y = 23, rot_y = 90 },
  puzzle_item_6 = { scale = 512, offset_y = 5, draws_at_pivot = true },
  examine_item_1 = { scale = 1300, offset_y = 4, rot_x = 90 },
})
