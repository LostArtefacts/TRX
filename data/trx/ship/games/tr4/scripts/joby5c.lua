trx.events.on_game_start(function()
  trx.objects.scaled_spikes.properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.EXTENDED
end)

require("tr4.inv_setup").apply({
  key_item_11 = { scale = 1200, offset_y = 5, rot_x = -90 },
  key_item_12 = { scale = 768, offset_y = 4, rot_x = -90 },
})
