trx.events.on_game_start(function()
  trx.items[3].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[4].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[87].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[61].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[100].properties.switch_mode = trx.items.SwitchMode.HIDDEN_PICKUP
  trx.items[101].properties.pickup_mode = trx.items.PickupMode.HIDDEN

  trx.objects.waterfall_1.properties.loop_sound = trx.items.WaterfallSound.SAND
  trx.objects.waterfall_1.properties.hide_when_inactive = true
  trx.objects.waterfall_2.properties.loop_sound = trx.items.WaterfallSound.SAND
  trx.objects.waterfall_2.properties.hide_when_inactive = true
  trx.items[0].properties.collidable_when_done = false
  trx.objects.scaled_spikes.properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.EXTENDED
end)

-- The caption the level opens with, which the level's strings carry.
require("common.legend").setup(function()
  return trx.locale.get("general/legend")
end)
