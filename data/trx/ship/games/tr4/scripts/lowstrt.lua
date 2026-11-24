local cutscenes = require("tr4.cutscenes")

local DETONATOR_CUTSCENE = 24
local DETONATOR_ROOM = 107
-- Lara holds her pistols from the start of the scene until this frame.
local HOLSTER_FRAME = 552

-- Moves both of Lara's pistols between her hands and her holsters.
local function toggle_pistols()
  local lara = trx.lara.item
  lara:run_action(trx.catalog.flip_effects.LARA_DRAW_RIGHT_GUN)
  lara:run_action(trx.catalog.flip_effects.LARA_DRAW_LEFT_GUN)
end

trx.events.on_game_start(function()
  trx.items[45].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[7].properties.flash_room_nums = { 87, 22, 10, 15 }
  trx.items[11].properties.flash_room_nums = { 87, 22, 10, 15 }

  trx.objects.animating_13.properties.collidable = false
  trx.objects.animating_14.properties.collidable = false
  trx.objects.animating_16.properties.collidable = false
  trx.objects.switch_type_generic_1.properties.switch_mode =
    trx.items.SwitchMode.SHOVE
end)

require("tr4.inv_setup").apply({
  puzzle_item_1 = { scale = 768, offset_y = 4 },
  puzzle_item_2 = { scale = 1024, rot_x = 180, rot_y = 90, rot_z = 90 },
  puzzle_item_4 = { scale = 768, rot_x = -67.5, rot_y = 90, rot_z = -112.5 },
  puzzle_item_5 = { scale = 512, rot_x = -90, rot_y = 180 },
  puzzle_item_8 = { scale = 1024, offset_y = 8, rot_y = 180 },
  puzzle_item_1_combo_1 = { scale = 768, offset_y = 4, draws_at_pivot = true },
  puzzle_item_1_combo_2 = { scale = 768, offset_y = 2 },
  puzzle_item_2_combo_1 = { scale = 1024, rot_x = 180, rot_y = 90, rot_z = 90 },
  puzzle_item_2_combo_2 = { scale = 768, rot_x = 180, rot_y = 90, rot_z = 90 },
  puzzle_item_8_combo_1 = { scale = 1024, offset_y = 8, rot_y = 180 },
  puzzle_item_8_combo_2 = {
    scale = 640,
    offset_y = 4,
    rot_x = -90,
    rot_y = 180,
  },
})

cutscenes.register(DETONATOR_CUTSCENE, {
  chat = {
    {
      lara = true,
      ranges = {
        { 91, 122 },
        { 139, 172 },
        { 504, 552 },
        { 565, 637 },
        { 648, 678 },
        { 1596, 1616 },
        { 1774, 1821 },
      },
    },
    {
      actor = 1,
      node = 14,
      ranges = {
        { 209, 490 },
        { 701, 975 },
        { 1146, 1583 },
        { 1654, 1768 },
        { 1839, 1879 },
        { 1893, 1930 },
        { 1940, 2160 },
      },
      speech_heads = {
        trx.catalog.objects.actor_1_speech_head_1,
        trx.catalog.objects.actor_1_speech_head_2,
      },
    },
  },
  hide = { trx.catalog.objects.animating_13 },
  on_start = toggle_pistols,
  frames = { [HOLSTER_FRAME] = toggle_pistols },
  on_end = function()
    local items = trx.items.query
      :of_object(trx.catalog.objects.puzzle_item_8_combo_1)
      :in_room(DETONATOR_ROOM)
      :matches()
    for _, item in ipairs(items) do
      item:trigger()
    end
  end,
})
