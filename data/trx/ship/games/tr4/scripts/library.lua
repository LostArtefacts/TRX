trx.events.on_game_start(function()
  trx.items[12].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[135].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[79].properties.pickup_mode = trx.items.PickupMode.CROWBAR
  trx.items[140].properties.pickup_mode = trx.items.PickupMode.CROWBAR
  trx.items[141].properties.pickup_mode = trx.items.PickupMode.CROWBAR

  local expanding_blocks = require("tr4.expanding_blocks")
  expanding_blocks.initialise(trx.objects.raising_block_1, true, false)
  expanding_blocks.initialise(trx.objects.raising_block_2, true, false)
end)
