trx.events.on_game_start(function()
  trx.items[52].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[59].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[60].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[69].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[95].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[96].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[97].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[62].properties.lift = true
  trx.items[112].properties.switch_mode = trx.items.SwitchMode.HIDDEN_PICKUP
  trx.items[119].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[122].properties.switch_mode = trx.items.SwitchMode.HIDDEN_PICKUP
  trx.items[126].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[130].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  -- TODO: The burning torch has no TRX object yet
  -- trx.items[113].properties.pickup_mode = trx.items.PickupMode.HIDDEN
  trx.items[121].properties.pickup_mode = trx.items.PickupMode.HIDDEN
  trx.items[125].properties.pickup_mode = trx.items.PickupMode.HIDDEN
  trx.items[129].properties.pickup_mode = trx.items.PickupMode.HIDDEN
end)

require("tr4.inv_setup").apply({
  puzzle_item_6 = { scale = 768, offset_y = 3 },
  puzzle_item_7 = { scale = 768, offset_y = 9 },
})
