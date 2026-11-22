local cutscenes = require("tr4.cutscenes")

local HALL_CUTSCENE = 13

require("tr4.inv_setup").apply({
  key_item_2 = { scale = 1024, rot_x = -90 },
  key_item_3 = { scale = 1024, rot_x = -90 },
  puzzle_item_1 = { scale = 1280 },
  puzzle_item_2 = { scale = 800, offset_y = 1 },
  puzzle_item_3 = { scale = 800, offset_y = 1, rot_y = 180 },
  puzzle_item_1_combo_1 = { scale = 384 },
  puzzle_item_1_combo_2 = { scale = 1200 },
})

cutscenes.register(HALL_CUTSCENE, {
  on_start = function()
    trx.cutscenes.set_lara_return({ x = 79194, y = 0, z = 40951 }, -20741)
  end,
})
