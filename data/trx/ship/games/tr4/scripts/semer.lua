trx.events.on_game_start(function(is_save)
  trx.items[47].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[235].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[236].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[237].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[238].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[50].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[51].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[52].properties.switch_mode = trx.items.SwitchMode.HIDDEN_PICKUP
  trx.items[57].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[159].properties.switch_mode = trx.items.SwitchMode.HIDDEN_PICKUP
  trx.items[182].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[183].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[184].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[189].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[190].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[191].properties.switch_mode = trx.items.SwitchMode.HIDDEN_PICKUP
  trx.items[210].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[211].properties.switch_mode = trx.items.SwitchMode.HIDDEN_PICKUP
  trx.items[213].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[53].properties.pickup_mode = trx.items.PickupMode.HIDDEN
  -- TODO: the burning torch has no TRX object yet
  -- trx.items[160].properties.pickup_mode = trx.items.PickupMode.HIDDEN
  trx.items[188].properties.pickup_mode = trx.items.PickupMode.HIDDEN
  trx.items[192].properties.pickup_mode = trx.items.PickupMode.HIDDEN
  trx.items[212].properties.pickup_mode = trx.items.PickupMode.HIDDEN

  trx.objects.waterfall_1.properties.hide_when_inactive = true
  trx.objects.waterfall_2.properties.hide_when_inactive = true

  trx.items[79].properties.use_idle_pose = true
  trx.items[80].properties.use_idle_pose = true

  require("tr4.expanding_blocks").initialise(
    trx.objects.raising_block_1,
    true,
    not is_save
  )
end)

require("tr4.inv_setup").apply({
  puzzle_item_4 = { scale = 1024 },
  puzzle_item_5 = { scale = 1024 },
  puzzle_item_4_combo_1 = { scale = 1024 },
  puzzle_item_4_combo_2 = { scale = 1024 },
  examine_item_2 = { scale = 1280 },
})
