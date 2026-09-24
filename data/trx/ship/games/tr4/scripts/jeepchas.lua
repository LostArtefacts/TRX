trx.events.on_game_start(function()
  trx.objects.scaled_spikes.properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.EXTENDED
  trx.objects.guide.properties.guides_lara = false
end)

require("tr4.inv_setup").apply({
  puzzle_item_1 = {
    scale = 1024,
    offset_y = 8,
    rot_x = 67.5,
    rot_y = 45,
    rot_z = 90,
  },
})
