local cutscenes = require("tr4.cutscenes")

-- No floor trigger names this one; it opens the level.
local ARRIVAL = 12

cutscenes.play_on_start(ARRIVAL)

-- The jeep Lara arrives in is a level item standing where the scene drives its
-- own, so the level's one waits until the scene has run.
cutscenes.register(ARRIVAL, {
  on_start = function()
    cutscenes.set_items_visible(trx.catalog.objects.animating_6, false)
    -- She rides in on the jeep, and her shadow is the one the scene casts for
    -- it, so it is far wider than the one she casts on foot.
    trx.cutscenes.set_lara_shadow_bounds({
      min_x = -600,
      min_y = -777,
      min_z = -600,
      max_x = 600,
      max_y = 1,
      max_z = 600,
    })
  end,
  on_end = function()
    cutscenes.set_items_visible(trx.catalog.objects.animating_6, true)
  end,
})

trx.events.on_game_start(function(is_save)
  trx.items[66].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[68].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[82].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[85].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.objects.switch_type_generic_1.properties.switch_mode =
    trx.items.SwitchMode.HIDDEN_REACH
  trx.objects.switch_type_generic_2.properties.switch_mode =
    trx.items.SwitchMode.SHOVE

  require("tr4.expanding_blocks").initialise(
    trx.objects.raising_block_2,
    true,
    not is_save
  )
end)

require("tr4.inv_setup").apply({
  key_item_2 = { scale = 1024, rot_x = -90 },
  puzzle_item_1 = { scale = 1280 },
  puzzle_item_2 = { scale = 800, offset_y = 1 },
  puzzle_item_3 = { scale = 800, offset_y = 1 },
  puzzle_item_1_combo_1 = { scale = 384 },
  puzzle_item_1_combo_2 = { scale = 1200 },
})
