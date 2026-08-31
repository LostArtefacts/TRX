trx.events.on_game_start(function()
  trx.items[6].properties.requires_heavy_trigger = true
  trx.items[8].properties.requires_heavy_trigger = true
  trx.items[9].properties.requires_heavy_trigger = true
  trx.items[68].properties.requires_heavy_trigger = true
  trx.items[69].properties.requires_heavy_trigger = true
  trx.items[71].properties.requires_heavy_trigger = true
  trx.items[89].properties.requires_heavy_trigger = true
  trx.items[91].properties.requires_heavy_trigger = true
  trx.items[93].properties.requires_heavy_trigger = true
  trx.items[95].properties.requires_heavy_trigger = true
  trx.objects.scaled_spikes.properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.EXTENDED
end)
