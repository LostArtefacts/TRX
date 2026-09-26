trx.events.on_game_start(function()
  trx.objects.animating_16.properties.collidable = false
  trx.items[26].properties.crowbar = true
  trx.items[123].properties.crowbar = true
  trx.items[11].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[124].properties.switch_mode = trx.items.SwitchMode.HIDDEN_PICKUP
  trx.items[126].properties.switch_mode = trx.items.SwitchMode.HIDDEN_PICKUP
  trx.items[125].properties.pickup_mode = trx.items.PickupMode.HIDDEN
  trx.items[127].properties.pickup_mode = trx.items.PickupMode.HIDDEN
  trx.items[116].properties.pickup_mode = trx.items.PickupMode.CROWBAR
  trx.items[5].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[6].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[7].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[8].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[9].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[10].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[13].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[38].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[57].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[60].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[68].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[69].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[70].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[71].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[118].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[122].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.objects.scaled_spikes.properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.ONE_SHOT
end)

require("tr4.inv_setup").apply({
  puzzle_item_5 = { scale = 1536, offset_y = 8, rot_x = -22.5, rot_y = 180 },
  puzzle_item_10 = { scale = 1200, offset_y = 19 },
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
