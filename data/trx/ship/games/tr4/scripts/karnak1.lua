local cutscenes = require("tr4.cutscenes")

-- No floor trigger names this one; it opens the level.
local ARRIVAL = 12

cutscenes.play_on_start(ARRIVAL)

-- The jeep Lara arrives in is a level item standing where the scene drives its
-- own, so the level's one waits until the scene has run.
cutscenes.register(ARRIVAL, {
  on_start = function()
    cutscenes.set_items_visible(trx.catalog.objects.animating_6, false)
  end,
  on_end = function()
    cutscenes.set_items_visible(trx.catalog.objects.animating_6, true)
  end,
})

trx.events.on_game_start(function()
  trx.items[66].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[68].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[82].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[85].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.objects.switch_type_generic_1.properties.switch_mode =
    trx.items.SwitchMode.HIDDEN_REACH
  trx.objects.switch_type_generic_2.properties.switch_mode =
    trx.items.SwitchMode.SHOVE
end)
