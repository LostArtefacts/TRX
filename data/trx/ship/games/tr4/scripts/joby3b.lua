trx.events.on_game_start(function()
  trx.objects.switch_type_generic_2.properties.switch_mode =
    trx.items.SwitchMode.SHOVE
  trx.items[159].properties.pickup_mode = trx.items.PickupMode.CROWBAR
  trx.objects.scaled_spikes.properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.EXTENDED
end)

require("tr4.inv_setup").apply({
  key_item_11 = { scale = 1200, offset_y = 5, rot_x = -90 },
  key_item_12 = { scale = 768, offset_y = 4, rot_x = -90 },
  puzzle_item_5 = { scale = 1200, offset_y = 23, rot_y = 90 },
  puzzle_item_6 = { scale = 512, offset_y = 5, draws_at_pivot = true },
})
