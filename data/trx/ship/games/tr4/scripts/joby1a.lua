trx.events.on_game_start(function()
  trx.objects.switch_type_generic_1.properties.switch_mode =
    trx.items.SwitchMode.SHOVE
end)

require("tr4.inv_setup").apply({
  key_item_1 = { scale = 768, offset_y = 4, rot_x = -90 },
  key_item_11 = { scale = 1200, offset_y = 5, rot_x = -90 },
  key_item_12 = { scale = 768, offset_y = 4, rot_x = -90 },
  puzzle_item_1 = {
    scale = 2560,
    offset_y = 3,
    rot_x = -135,
    rot_y = 90,
    rot_z = 90,
  },
  puzzle_item_5 = { scale = 1200, offset_y = 23, rot_y = 90 },
  puzzle_item_6 = { scale = 512, offset_y = 5, draws_at_pivot = true },
  puzzle_item_1_combo_1 = {
    scale = 1536,
    offset_y = 1,
    rot_x = 180,
    rot_y = 90,
    rot_z = 90,
  },
  puzzle_item_1_combo_2 = {
    scale = 2560,
    offset_y = 3,
    rot_x = -135,
    rot_y = 90,
    rot_z = 90,
  },
})
